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

#define UPG_FILE        SDMMC_MNT_PT"/upgrade.upg"
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

static int upg_exist(void)
{
    int r,flen;
    handle_t fl;
    fw_hdr_t h1,h2;
    int exist=0;
    
    r = dal_sd_status();
    if(r) {  //sd exist
        return -1;
    }
    
    r = fs_exist(UPG_FILE);
    if(r) {
        flen = fs_length(UPG_FILE);
        if(flen>0) {
            upgrade_get_fw_info(FW_CUR, &h1);
            fs_load(UPG_FILE, &h2, sizeof(h2));
            
            if(memcmp(&h1.fw, &h2.fw, sizeof(fw_info_t))) {
                LOGD("___ old.magic: 0x%08x, new.magic: 0x%08x\n",  h1.fw.magic, h2.fw.magic);
                LOGD("___ old.version: %s, new.version: %s\n",      h1.fw.version, h2.fw.version);
                LOGD("___ old.bldtime: %s, new.bldtime: %s\n",      h1.fw.bldtime, h2.fw.bldtime);
                LOGD("___ old.length: %d, new.length: %d\n",        h1.fw.length, h2.fw.length);
                
                exist = 1;
            }
        }
        
        fs_close(fl);
    }
    
    return exist;
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
static osThreadId_t conn_thread_id;
static void task_conn_fn(void *arg)
{
    int flag;
    #define CONN_MAX  3
    #define REST_MAX  3
    
    memset(&connHandle, 0, sizeof(connHandle));
    while(1) {
        
        if(connHandle.conn!=CONN_SUCCESS && paras_get_mode()==MODE_NORM && cap_is_finished()) {
        //if(connHandle.conn!=CONN_SUCCESS && paras_get_mode()==MODE_NORM) {
            connHandle.conn = CONN_BUSYING;
            
            LOGD("___ api_comm_connect\n");
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
                LOGD("___ at module reset, %d\n",connHandle.rst_cnt);
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


static void sd_upgrade_check(void)
{
    int r,index=0;
    int is_exist=0;
    U8 *pbuf=NULL;
    handle_t fl,fs;
    int rlen,flen,upg_ok_flag=0;
    
    is_exist = upg_exist();
    if(!is_exist) {
        LOGE("___ no upgrade.upg file exist\n");
        return;
    }
        
    pbuf = (U8*)malloc(TMP_LEN);
    if(!pbuf) {
        LOGE("____ sd_upgrade_check malloc failed\n");
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
        LOGD("____ reboot now ...\n");
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
    worktime_t wtime=paras_get_worktime();
        
    if(smp->mode==MODE_CALI || smp->pwrmode!=PWR_PERIOD_PWRDN) {
        //LOGW("___ in cali mode, power off disabled!\n");
        return;
    }
    
    finish = api_send_is_finished();
    if(!finish) {
        //LOGW("___ capture is not finished, pwroff disable!\n");
        return;
    }
    
    //以上都完成时才可以关机
    runtime = rtc2_get_runtime();
    LOGD("____ psrc: %d, finish: %d, runtime: %lu, invtime: %d\n", var->psrc, finish, runtime, smp->invTime);
    
    if(var->psrc==PWRON_RTC) {      //rtc poweron
        
        //finish=2: 发送完毕    finish=1: 发送未完成
        if((wtime.rtmin==0) || (runtime>wtime.rtmin))  {
            if(((finish==2) || ((finish==1) && (runtime>=wtime.rtmax)))) {
                api_comm_disconnect();      //断开连接, 该操作耗时较长
                api_send_save_file();
                
                countdown = smp->invTime- runtime;
                if(countdown>=COUNTDOWN_MIN) {
                    countdown -= 2;     //减2为修正值
                }
                else {
                    countdown = smp->invTime-2;
                }
                
                r = rtc2_set_countdown(countdown);
            }
        }
    }
    else if(var->psrc==PWRON_MANUAL) {                  //manual poweron
        //delay powerdown??
    }
    else if(var->psrc==PWRON_TIMER) {
        //?
    }
}


static F32 get_temp(dal_adc_data_t *adc)
{
    int r;
    F32 temp;
    
#ifdef USE_PT1000
    pt1000_temp(adc->t_pt, &temp);
#else
    rtc2_get_temp(&temp);
#endif
    
    return temp;
}

static void stat_polling(void)
{
    int r;
    U32 ts;
    F32 temp,invtime;
    stat_data_t stat;
    stat_info_t si;
    dal_adc_data_t da;
    static U32 send_tm=0;
    mqtt_pub_para_t pub_para={DATA_LT};

#if 1    
    invtime = dal_get_tick()-send_tm;
    if(send_tm>0 && invtime<(3600*24)) {
        return;
    }
#endif
    
    r = aiot_at_stat(&si);                  //信号强度和质量
    if(r) {
        return;
    }
    dal_adc_read(&da);
    
    stat.rssi = si.rssi;
    stat.ber  = si.ber;
    stat.rsrp = si.rsrp;
    stat.snr  = si.snr;
    stat.temp = get_temp(&da);

    stat.vbat = da.vbat;
    stat.time = rtc2_get_timestamp_ms();
    LOGD("___ temp: %.1f, vbat: %.1f, rssi: %d, ber: %d, rsrp: %d, snr: %d\n", stat.temp, stat.vbat, stat.rssi, stat.ber, stat.rsrp, stat.snr);
    
    if(api_comm_is_connected()) {
        r = comm_send_data(tasksHandle.hnet, &pub_para, TYPE_STAT, 0, &stat, sizeof(stat));
        if(r==0) {
            send_tm = dal_get_tick();
        }
        LOGD("___ stat send %s, tm: %d\n", r?"failed":"ok", invtime);
    }
}

////////////////////////////////////////////////////////////////
static void task_wdg_fn(void *arg)
{
#ifdef USE_WDG
    dal_wdg_init(WDG_TIME);
    
    while(1) {
        //LOGD("___%d\n", dal_get_tick_ms());
        dal_wdg_feed();     //喂狗
        osDelay(1000);
    }
#endif
}
static void task_stat_fn(void *arg)
{
    while(1) {
        stat_polling();
        osDelay(1000);
    }
}
static void task_pwroff_fn(void *arg)
{
    while(1) {
        pwroff_polling();
        osDelay(1000);
    }
}


void api_polling_conn_wait(void)
{
     while(connHandle.conn==CONN_BUSYING) osDelay(2);
}
void api_polling_conn_quit(void)
{
     while(connHandle.conn==CONN_BUSYING) osDelay(2);
}


void api_polling_start(void)
{
    sd_upgrade_check();
    
    start_task_simp(task_conn_fn, 4*KB, NULL, NULL);
    
    start_task_simp(task_wdg_fn,    256, NULL, NULL);
    start_task_simp(task_stat_fn,   2*KB, NULL, NULL);
    start_task_simp(task_pwroff_fn, 2*KB, NULL, NULL);
}
#endif


