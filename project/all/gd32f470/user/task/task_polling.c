#include "task.h"
#include "list.h"
#include "fs.h"
#include "cfg.h"
#include "dal.h"
#include "date.h"
#include "power.h"
#include "dal_rtc.h"
#include "dal_sd.h"
#include "upgrade.h"
#include "paras.h"
#include "dal_adc.h"
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

static U32 get_run_time(void)
{
    U32 t1,t2;
    date_time_t dt1,dt2;
    
    paras_get_datetime(&dt1);
    dal_rtc_get(&dt2);
    
    get_timestamp(&dt1, &t1);
    get_timestamp(&dt2, &t2);
    
    return (t2-t1);
}
static void poweroff_polling(void)
{
    smp_para_t *smp=paras_get_smp();
    
    if(smp->pwr_mode==0) {
        //是否正在写文件
        
        //缓存数据是否保存完毕
        
        //上传记录文件是否写入
        
        //4g模组是否关机
        
        //以上都完成时才可以关机
    }
    
}
static void stat_polling(void)
{
    //signal strength
    //battery level
    //temperature
    //
}












static void polling_tmr_callback(void *arg)
{
    task_post(TASK_POLLING, NULL, EVT_TIMER, 0, NULL, 0);
}

#define CONN_MAX  5
#define REST_MAX  5
static int conn_flag=0;
static osThreadId_t connThdId=NULL;
static void task_conn_fn(void *arg)
{
    static int conn_cnt=0;
    static int rest_cnt=0;
    
    while(1) {
        conn_flag = api_comm_connect();
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
static void start_conn_task()
{
    smp_para_t *smp=paras_get_smp();
    
    //周期模式采样时，采样完成再启动连接，避免4g对信号造成干扰
    //if (smp->smp_mode==0 && api_cap_stoped() && !connThdId) {
    if(!conn_flag && !connThdId) {
        start_task_simp(task_conn_fn, 2048, NULL, &connThdId);
    }
    
    if(conn_flag) {
        api_comm_send_para();
    }
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
                    
                    stat_polling();
                    poweroff_polling();
                    //upgrade_polling();                   
                    
                    start_conn_task();
                }
                break;
                
                case EVT_SAVE:
                {
                    paras_save();
                }
                break;
            }
        }
        
        task_yield();
    }
}
#endif


