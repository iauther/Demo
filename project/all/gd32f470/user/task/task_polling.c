#include "task.h"
#include "list.h"
#include "fs.h"
#include "cfg.h"
#include "dal.h"
#include "date.h"
#include "power.h"
#include "rtc.h"
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


static U32 tmrCount=0;
static handle_t fList=NULL;
extern at_sig_stat_t sig_stat;

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
    int finish=0;
    U32 runtime,countdown;
    gbl_var_t  *var=&allPara.var;
    smp_para_t *smp=paras_get_smp();
    
    if(paras_get_mode()==MODE_CALI) {
        LOGW("___ in cali mode, power off disabled!\n");
        return;
    }
    
    if(smp->pwrmode==SMP_PERIOD_MODE) {
        //���������Ƿ񱣴����
        finish = api_nvm_is_finished();
        if(!finish) {
            LOGW("___ capture is not finished�� pwroff disable!\n");
            return;
        }
        
        //���϶����ʱ�ſ��Թػ�
        runtime = rtc2_get_runtime();
        countdown = (smp->worktime>(runtime+3))?(smp->worktime- runtime):3;
        LOGD("____ psrc: %d, finish: %d, runtime: %lu, worktime: %d, countdown: %d\n", var->psrc, finish, runtime, smp->worktime, countdown);
        
        if(var->psrc==1) {      //rtc poweron
            if(((finish==2) || ((finish==1) && (runtime>=30))) && (countdown>=3)) {
                at_hal_power(0);            //��4gģ��
                
                countdown -= 2;     //��2Ϊ����ֵ
                LOGD("____ rtc2_set_countdown, %d\n", countdown);
                rtc2_set_countdown(countdown);
                while(1);
            }
        }
        else {                  //manual poweron
            //delay powerdown??
        }
    }
    
}
static void stat_polling(void)
{
    //signal strength
    //battery level
    //temperature
    //
}
static void conn_polling()
{
    smp_para_t *smp=paras_get_smp();
    
    if(!cap_is_finished() || (paras_get_mode()==MODE_CALI)) {
        return;
    }
    
    //����ģʽ����ʱ������������������ӣ�����4g���ź���ɸ���
    if (!conn_flag  && !connThdId) {
        start_task_simp(task_conn_fn, 2048, NULL, &connThdId);
    }
    
    if(conn_flag) {
        //api_comm_send_para();
    }
}
////////////////////////////////////////////////////////////////
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
                    tmrCount++;
                    
                    dal_wdg_feed();     //ι��
                    
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


