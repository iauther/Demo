#include "task.h"
#include "list.h"
#include "fs.h"
#include "cfg.h"
#include "dal.h"
#include "datetime.h"
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
#include "dal_gpio.h"
#include "aiot_at_api.h"
#include "hal_at_impl.h"
#include "cfg.h"


#define POLL_TIME       1000

#define UPG_FILE        SFLASH_MNT_PT"/app.upg"
#define TMP_LEN         (0x100*8)



#ifdef OS_KERNEL

typedef struct {
    int     id;
    void    *data;
}file_node_t;

static handle_t hpwr=NULL;
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

enum {
    CONN_STOP=0,
    CONN_FAILED,
    CONN_BUSYING,
    CONN_SUCCESS,
};
typedef struct {
    int conn;    
    int conn_cnt;
    int rst_cnt;
}conn_t;

static conn_t connHandle;
static void task_conn_fn(void *arg)
{
#define CONN_MAX  5
#define REST_MAX  5
    int flag;
    
    memset(&connHandle, 0, sizeof(connHandle));
    while(1) {
        
        //if(connHandle.conn!=CONN_SUCCESS && paras_get_mode()==MODE_NORM && cap_is_finished()) {
        if(connHandle.conn!=CONN_SUCCESS && paras_get_mode()==MODE_NORM) {
            connHandle.conn = CONN_BUSYING;
            flag = api_comm_connect(paras_get_port());
            if(flag) {
                connHandle.conn_cnt = 0;
                connHandle.rst_cnt = 0;
                connHandle.conn = CONN_SUCCESS;
                LOGD("___ api_comm_connect ok!\n");
            }
            else {
                connHandle.conn = CONN_FAILED;
                LOGD("___ api_comm_connect fail!\n");
            }

#if 1
            connHandle.conn_cnt++;
            LOGD("___ ad module conn times: %d\n", connHandle.conn_cnt);
            
            if(connHandle.conn_cnt>0 && (connHandle.conn_cnt%CONN_MAX==0)) {
                hal_at_reset();
                connHandle.rst_cnt++;
                LOGD("___ ad module reset: %d\n", connHandle.rst_cnt);
                
                if(connHandle.rst_cnt>0 && (connHandle.rst_cnt%REST_MAX==0)) {
                    LOGE("___ ad module reset reach % times, please check the module and repair it\n", connHandle.rst_cnt);
                }
            }
#endif
        }
        
        osDelay(1000);
    }
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


static void print_time(char *s, datetime_t *dt)
{
    LOGD("%s %4d.%02d.%02d-%02d-%02d:%02d:%02d\n", s, dt->date.year,dt->date.mon,dt->date.day,
                                                   dt->date.week,dt->time.hour,dt->time.min,dt->time.sec);
}
static void die(void)
{
    LOGD("___ die to wait power off...\n");
    while(1);
}
static void pwroff_polling(void)
{
    U32 runtime;
    int r,finish=0,countdown;
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
            LOGW("___ capture is not finished, pwroff disable!\n");
            return;
        }
        
        //以上都完成时才可以关机
        r = rtc2_get_runtime(&runtime);
        if(r==0) {      //获取运行时间成功才可以进行下面流程
            LOGD("____ psrc: %d, finish: %d, runtime: %lu, workInterval: %d\n", var->psrc, finish, runtime, smp->workInterval);
            
            if(var->psrc==PWRON_RTC) {      //rtc poweron
                if((finish==2) || ((finish==1) && (runtime>=RUNTIME_MAX))) {
                    //api_comm_disconnect();      //断开连接, 该操作耗时较长
                    
                    countdown = smp->workInterval- runtime;
                    
                    if(countdown>=COUNTDOWN_MIN) {
                        countdown -= 2;     //减2为修正值
                    }
                    else {
                        countdown = smp->workInterval;
                    }
                    
                    r = rtc2_set_countdown(countdown);
                    if(r==0) {
                        //添加拉高timer管脚的操作
                        dal_gpio_set_hl(hpwr, 1);           //v136版本增加了一个硬件timer, 关机时需要拉高一下连接timer的引脚
                        die();
                    }
                }
            }
            else if(var->psrc==PWRON_MANUAL) {                  //manual poweron
                //delay powerdown??
            }
        }
    }
    
}


void rtc_test(void)
{
    int i,r,cnt=0;
    U32 runtime,countdown;

#if 1
    #undef RUNTIME_MAX
    #undef COUNTDOWN_MIN
    
    #define RUNTIME_MAX     3
    #define COUNTDOWN_MIN   3
#endif
    
    while(1) {
        r = rtc2_get_runtime(&runtime);
        if(r==0) {      //获取运行时间成功才可以进行下面流程
            
            LOGD("___ runtime: %d\n", runtime);
            if((runtime>=RUNTIME_MAX)) {
                
                countdown = COUNTDOWN_MIN;
                LOGD("____ countdown: %d\n", countdown);
                
                r = rtc2_set_countdown(countdown);
                if(r==0) {
                    LOGD("____ die to power off\n");
                    while(1);
                }
            }
        }
        
        dal_delay_ms(1000);
    }
}

void pwroff_polling_test(void)
{
    rtc_test();
    //rtc2_test();
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
    signal_info_t si;
    dal_adc_data_t da;
    static U8 send_flag=0;
    mqtt_pub_para_t pub_para={DATA_LT};
    
    if(!api_comm_is_connected() || send_flag==1) {
        return;
    }
    
    r = hal_at_stat(&si);                  //信号强度和质量
    if(r) {
        return;
    }
    LOGE("___ at_hal_stat ok\n");
    
    dal_adc_read(&da);
    
    stat.rssi = si.rssi;
    stat.ber  = si.ber;
#ifdef USE_PT1000
    stat.temp = get_temp(&da);
#else
    stat.temp = da.t_in;
#endif
    stat.vbat = da.vbat;
    stat.time = rtc2_get_timestamp_ms();
    LOGD("___ pt: %.1fv, temp: %.1f, vbat: %.1f\n", da.t_pt, stat.temp, stat.vbat);
    
    r = comm_send_data(tasksHandle.hconn, &pub_para, TYPE_STAT, 0, &stat, sizeof(stat));
    if(r==0) {
        send_flag = 1;
    }
}

////////////////////////////////////////////////////////////////
static void task_wdg_fn(void *arg)
{
#ifdef USE_WDG
    dal_wdg_init(WDG_TIME);
    
    while(1) {
        dal_wdg_feed();     //喂狗
        osDelay(2000);
    }
#endif
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
    dal_gpio_cfg_t gc;
    task_handle_t *h=(task_handle_t*)arg;
    
    gc.gpio.grp = GPIOA;
    gc.gpio.pin = GPIO_PIN_0;
    gc.mode = MODE_OUT_PP;
    gc.pull = PULL_NONE;
    gc.hl = 0;;
    
    LOGD("_____ task_polling running\n");
    
    start_task_simp(task_wdg_fn, 256, NULL, NULL);
#ifdef BOARD_V136
    hpwr = dal_gpio_init(&gc);
#endif
    htmr = task_timer_init(polling_tmr_callback, NULL, POLL_TIME, -1);
    if(htmr) {
        task_timer_start(htmr);
    }
    start_task_simp(task_conn_fn, 4*KB, NULL, NULL);
    
    while(1) {
        r = task_recv(TASK_POLLING, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
#if 1
                    //api_cap_start_all();        //finish后不可启动
                    //stat_polling();
                    //pwroff_polling();
#else
                    pwroff_polling_test();
#endif
                }
                break;
                
                case EVT_CALI:
                {
                    cali_sig_t *sig=(cali_sig_t*)e.data;
                    
                    LOGD("___ cali para, ch: %hhu, lv: %hhu, max: %hhu, seq: %hhu, volt: %.1fmv, freq: %ukhz, bias: %.1fmv\n", sig->ch, sig->lv, sig->max, sig->seq, sig->volt, sig->freq, sig->bias);
                    r = paras_set_cali_sig(sig->ch, sig);
                    if(r==0) {
                        api_cap_stop(sig->ch);
                        paras_set_mode(MODE_CALI);
                        
                        //断4g并关断
                        while(connHandle.conn==CONN_BUSYING) osDelay(2);
                        api_comm_disconnect();
                        
                        api_cap_start(sig->ch);
                    }
                    
                }
                break;
            }
        }
        
        //task_yield();
    }
}
#endif


