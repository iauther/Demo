#include "task.h"
#include "list.h"
#include "fs.h"
#include "cfg.h"
#include "dal.h"
#include "date.h"
#include "power.h"
#include "rtc.h"
#include "comm.h"
#include "protocol.h"
#include "pt1000.h"
#include "dal_sd.h"
#include "upgrade.h"
#include "paras.h"
#include "dal_adc.h"
#include "dal_wdg.h"
#include "dal_delay.h"
#include "aiot_at_api.h"
#include "hw_at_impl.h"


#define POLL_TIME       1000

#define UPG_FILE        SFLASH_MNT_PT"/app.upg"
#define TMP_LEN         (0x100*8)


#ifdef OS_KERNEL

typedef struct {
    int     id;
    void    *data;
    
}file_node_t;


static int cap_is_finished(void)
{
    U8 ch;
    
    for(ch=0; ch<CH_MAX; ch++) {
        if(!paras_is_finished(ch)) {
            return 0;
        }
    }
    
    return 1;
}


static int upg_file_is_exist(void)
{
    int r,flen;
    handle_t fl;
    fw_info_t info1,info2;
    int upg_file_exist=0;
    
    r = dal_sd_status();
    if(r==0) {  //sd exist
        
        fl = fs_open(UPG_FILE, FS_MODE_RW);
        if(fl) {
            flen = fs_size(fl);
            if(flen>0) {
                
                fs_read(fl, &info1, sizeof(info1));
                upgrade_get_fw_info(&info2);
        
                if(memcmp(&info1, &info2, sizeof(fw_info_t))) {
                    upg_file_exist = 1;
                }   
            }
            
            fs_close(fl);
        }
    }
    
    return upg_file_exist;
}

static int conn_flag=0;
static osThreadId_t connThdId=NULL;
static void task_conn_fn(void *arg)
{
#define CONN_MAX  5
#define REST_MAX  5
    static int conn_cnt=0;
    static int rest_cnt=0;
    U8 port=paras_get_port();
    
    while(1) {
        conn_flag = api_comm_connect(port);
        if(conn_flag) {
            conn_cnt = 0;
            LOGD("___ api_comm_connect ok!\n");
            break;
        }
        
        conn_cnt++;
        LOGD("___ ad module conn times: %d\n", conn_cnt);
        
        if(conn_cnt>0 && (conn_cnt%CONN_MAX==0)) {
            at_hal_reset();
            rest_cnt++;
            LOGD("___ ad module reset: %d\n", rest_cnt);
            
            if(rest_cnt>0 && (rest_cnt%REST_MAX==0)) {
                LOGE("___ ad module reset reach % times, please check the module and repair it\n", rest_cnt);
            }
        }
    }
    LOGD("___ quit task_conn_fn ...\n");
		
    connThdId = NULL;
    osThreadExit();
}


static void upgrade_polling(U8 loop)
{
    int r,index=0;
    int is_exist=0;
    U8 *pbuf=NULL;
    handle_t fl,fs;
    static int upg_chk_flag=0;
    int rlen,flen,upg_ok_flag=0;
    
    if(!upg_chk_flag) {
        is_exist = upg_file_is_exist();
        if(!is_exist) {
            return;
        }
        
        if(!loop) {
            upg_chk_flag = 1;
        }
    }
        
    pbuf = (U8*)malloc(TMP_LEN);
    if(!pbuf) {
        LOGE("____ upgrade_proc malloc failed\n");
        return;
    }
        
    fl = fs_open(UPG_FILE, FS_MODE_RW);
    if(!fl) {
        LOGE("____ %s open failed\n", UPG_FILE);
        goto fail;
    }
    
    flen = fs_size(fl);
    if(flen<=0) {
        LOGE("____ upgrade file len wrong\n");
        goto fail;
    }
        
    LOGD("____ upgrade file len: %d\n", flen);
    upgrade_erase(flen);
    
    while(1) {
        rlen = fs_read(fl, pbuf, TMP_LEN);
        //LOGD("_____%d, 0x%02x, 0x%02x, \n", rlen, tmp[0], tmp[1]);
        if(rlen>0) {
            if(rlen<TMP_LEN) {
                r = upgrade_write(pbuf, rlen, -1);
                if(r) {
                    LOGE("____ upgrade failed\n");
                }
                else {
                    LOGD("____ upgrade ok\n");
                    upg_ok_flag = 1;
                }
                break;
            }
            
            r = upgrade_write(pbuf, rlen, index++);
            if(r) {
                LOGE("____ upgrade write failed, index: %d\n", index);
                break;
            }
        }
        else {
            LOGE("____ fs_read error\n");
            break;
        }
    }
    fs_close(fl);
    
    if(upg_ok_flag) {
        LOGD("____ reboot now ...\n\n\n");
        dal_reboot();
    }
    
fail:
    free(pbuf);
    fs_close(fl);
}


static void print_time(char *s, date_time_t *dt)
{
    LOGD("%s %4d.%02d.%02d-%02d-%02d:%02d:%02d\n", s, dt->date.year,dt->date.mon,dt->date.day,
                                                   dt->date.week,dt->time.hour,dt->time.min,dt->time.sec);
}
static void pwroff_polling(void)
{
    int r,finish=0;
    U32 runtime,countdown;
    gbl_var_t  *var=&allPara.var;
    smp_para_t *smp=paras_get_smp();
    
    if(paras_get_mode()==MODE_CALI) {
        //LOGW("___ in cali mode, power off disabled!\n");
        return;
    }
    
    if(smp->pwrmode==SMP_PERIOD_MODE) {
        //缓存数据是否保存完毕
        finish = api_nvm_is_finished();
        if(!finish) {
            LOGW("___ capture is not finished， pwroff disable!\n");
            return;
        }
        
        //以上都完成时才可以关机
        r = rtc2_get_runtime(&runtime);
        if(r==0) {      //获取运行时间成功才可以进行下面流程
            countdown = (smp->worktime>(runtime+3))?(smp->worktime- runtime):3;
            
            LOGD("____ psrc: %d, finish: %d, runtime: %lu, worktime: %d, countdown: %d\n", var->psrc, finish, runtime, smp->worktime, countdown);
            if(var->psrc==1) {      //rtc poweron
                if(((finish==2) || ((finish==1) && (runtime>=30))) && (countdown>=3)) {
                    
                    api_comm_disconnect();      //断开连接
                    at_hal_power(0);            //关4g模组
                    
                    countdown -= 2;     //减2为修正值
                    LOGD("____ rtc2_set_countdown, %d\n", countdown);
                    r = rtc2_set_countdown(countdown);
                    if(r==0) {
                        while(1);
                    }
                }
            }
            else {                  //manual poweron
                //delay powerdown??
            }
        }
    }
    
}
static F32 get_temp(dal_adc_data_t *adc)
{
    int r;
    F32 temp;
    
    pt1000_temp(adc->t_pt, &temp);
    
    return temp;
}

static void stat_polling(void)
{
    int r;
    F32 temp;
    stat_data_t stat;
    sig_stat_t sig;
    dal_adc_data_t da;
    static U8 send_flag=0;
    mqtt_pub_para_t pub_para={DATA_LT};
    
    if(!api_comm_is_connected() || send_flag==1) {
        return;
    }
    
    r = at_hal_stat(&sig);                  //信号强度和质量
    if(r) {
        LOGE("___ get sig stat failed\n");
        return;
    }
    
    dal_adc_read(&da);
    
    stat.rssi = sig.rssi;
    stat.ber  = sig.ber;
    //stat.temp = get_temp(&da);
    stat.temp = da.t_in;
    stat.vbat = da.vbat;
    stat.time = rtc2_get_timestamp_ms();
    LOGD("___ pt: %.1fv, temp: %.1f, vbat: %.1f\n", da.t_pt, stat.temp, stat.vbat);
    
    r = comm_send_data(tasksHandle.hconn, &pub_para, TYPE_STAT, 0, &stat, sizeof(stat));
    if(r==0) {
        send_flag = 1;
    }
}
static void conn_polling()
{
    smp_para_t *smp=paras_get_smp();
    
    if(!cap_is_finished() || (paras_get_mode()==MODE_CALI)) {
        return;
    }
    
    //周期模式采样时，采样完成再启动连接，避免4g对信号造成干扰
    if (!conn_flag  && !connThdId) {
        start_task_simp(task_conn_fn, 2048, NULL, &connThdId);
    }
    
    if(conn_flag) {
        //api_comm_send_para();
    }
}
////////////////////////////////////////////////////////////////
static void task_wdg_fn(void *arg)
{
    dal_wdg_init(WDG_TIME);
    
    while(1) {
        dal_wdg_feed();     //喂狗
        osDelay(2000);
    }
}
static void polling_tmr_callback(void *arg)
{
    task_post(TASK_POLLING, NULL, EVT_TIMER, 0, NULL, 0);
}

void task_polling_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    handle_t htmr;
    task_handle_t *h=(task_handle_t*)arg;
    
    LOGD("_____ task_polling running\n");

    start_task_simp(task_wdg_fn, 256, NULL, NULL);
    
    htmr = task_timer_init(polling_tmr_callback, NULL, POLL_TIME, -1);
    if(htmr) {
        task_timer_start(htmr);
    }
    
    while(1) {
        r = task_recv(TASK_POLLING, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    pwroff_polling();
                    
                    stat_polling();
                    //upgrade_polling();
                    
                    conn_polling();
                }
                break;
            }
        }
        
        task_yield();
    }
}
#endif


