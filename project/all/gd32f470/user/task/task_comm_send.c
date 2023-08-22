#include "comm.h"
#include "task.h"
#include "list.h"
#include "ecxxx.h"
#include "dal_rtc.h"
#include "dal_delay.h"
#include "fs.h"
#include "paras.h"



#define EV_SEND_CNT_THRESHOLD       200


#define FILE_MAX_LEN  (1024*1024)



#ifdef OS_KERNEL
static int comm_iterator_fn(handle_t l, node_t *data, node_t *node, void *arg, int *act)
{
    void *addr=(void*)(*(U32*)node->buf);
    
    comm_send_data(tasksHandle.hcom, addr, TYPE_CAP, 0, data->buf, data->dlen);
    *act = ACT_NONE;
    
    return 0;
}
static int comm_broadcast(handle_t list, node_t *data)
{
    return list_iterator(list, data, comm_iterator_fn, NULL);
}

static void tmr_trig_callback(void *arg)
{
    task_post(TASK_COMM_SEND, NULL, EVT_TIMER, 0, NULL, 0);
}
///////////////////////////////////////////////////////////////////////////

static handle_t xHandle=NULL;
static int data_save_bin(ch_data_t *pch)
{
    int r,i,x=-1;
    char time[30];
    char tmp[100];
    date_time_t dt;
    handle_t fs=0;
    int dayMin=(24*60);
    int flen,wlen,maxCnt,maxLen;
    
    maxCnt = dayMin/allPara.usr.smp.period;
    maxLen = FILE_MAX_LEN;//allPara.usr.chn[ch].samplePoints*4*maxCnt;
    
    if(!xHandle) {
        r = dal_rtc_get(&dt);

        sprintf(time, "%04d/%02d/%02d/%02d", dt.date.year, dt.date.mon, dt.date.day, pch->ch);
        sprintf(tmp, "%s/%s.bin", SDMMC_MNT_PT, time);
        
        flen = fs_length(tmp);
        if(flen<0 || flen>=maxLen) {
            LOGE("___ %s flen: %d\n", tmp, flen);
            return -1;
        }        
        
        xHandle = fs_open(tmp, FS_MODE_CREATE);
        if(!xHandle) {
            LOGE("___fs_open %s failed", tmp);
            return -1;
        }
        
        //应先写入时间戳和配置(采样率、位宽等)
        //
        //
    }
    
    if(xHandle) {
        wlen = fs_append(xHandle, pch->data, pch->dlen, 0);
        if(wlen>0) {
            if(fs_size(xHandle)>=maxLen) {
                fs_close(xHandle);
                xHandle = NULL;
            }
        }
    }
    
    return 0;
}

#define FILE_BUF_LEN  2000
static char fileBuf[FILE_BUF_LEN+10];
static int data_save_csv(ch_data_t *pch)
{
    int r,i,j;
    char time[30];
    char tmp[60];
    date_time_t dt;
    handle_t fs=0;
    int dayMin=(24*60);
    int flen,wlen,maxCnt,maxLen;
    fs_space_t sp;
    
    maxCnt = dayMin/allPara.usr.smp.period;
    maxLen = allPara.usr.ch[pch->ch].smpPoints*4*maxCnt;
    
    r = fs_get_space(SDMMC_MNT_PT, &sp);
    if(r==0 && sp.free<pch->dlen*10) {
        LOGE("___ SDMMC is full, total: %lld, free: %lld\n", sp.total, sp.free);
        return -1;
    }
    
    if(!xHandle) {
        r = dal_rtc_get(&dt);

        sprintf(time, "%04d/%02d/%02d/%02d", dt.date.year, dt.date.mon, dt.date.day, pch->ch);
        sprintf(tmp, "%s/%s.csv", SDMMC_MNT_PT, time);
        
        flen = fs_length(tmp);
        if(flen<0 || flen>=maxLen) {
            LOGE("___ %s flen: %d\n", tmp, flen);
            return -1;
        }        
        
        xHandle = fs_open(tmp, FS_MODE_CREATE);
        if(!xHandle) {
            LOGE("___fs_open %s failed", tmp);
            return -1;
        }
        
        //应先写入时间戳和配置(采样率、位宽等)
        sprintf(tmp, "\"ch:%d, freq: %d, time: %s\"\n", pch->ch, allPara.usr.ch[pch->ch].smpFreq, time);
        fs_append(xHandle, tmp, strlen(tmp), 0);
    }
    
    if(xHandle) {
        int dw=6;
        int onelen=FILE_BUF_LEN/dw;
        int times=pch->dlen/onelen;
        int left=pch->dlen%onelen;
        int offset;
        
        //一个浮点数占位dw个字节
        for(i=0; i<times; i++) {
            for(j=0; j<onelen; j++) {
                offset = i*onelen+j;
                sprintf(fileBuf+j*dw, "%1.03f\n", pch->data[offset]);
            }
            wlen = fs_append(xHandle, fileBuf, onelen*dw, 0); 
            if(wlen<=0) {
                return -1;
            }
        }
        
        offset = i*onelen;
        for(j=0; j<left; j++) {
            sprintf(fileBuf+j*dw, "%1.03f\n", pch->data[i+j]);
        }
        wlen = fs_append(xHandle, pch->data, left*dw, 0); 
        if(wlen<=0) {
            return -1;
        }
        
        if(fs_size(xHandle)>=maxLen) {
            fs_close(xHandle);
            xHandle = NULL;
        }
    }
    
    return 0;
}

//////////////////////////////////////////////////////////////
typedef struct {
    U8          x[12][31];
}flag_t;

typedef struct {
    date_s_t    date;
    flag_t      flag;
}my_flag_t;


extern int mqtt_pub_str(upload_data_t *up);
extern int mqtt_pub_raw(upload_data_t *up);
extern int mqtt_is_connected(void);

//check the unupload file and send it out
static int is_leap(U16 year)
{
    if(year%400==0 || ((year%4==0) && (year%100))) {
        return 1;
    }
    
    return 0;
}
static void date_inc(date_s_t *d)
{
    int i,leap;
    U8 day[12]={30,28,31,30,31,30,31,31,30,31,30,31};
    
    if(d->day<(day[d->mon]+leap)) {
        d->day++;
    }
    else {
        d->day = 0;
        
        if(d->mon<12) {
            d->mon++;
        }
        else {
            d->mon = 0;
            d->year++;
        }
    }
}
static int file_upload_chk(U8 ch, my_flag_t *mf)
{
    int r;
    int leap;
    char tmp[64];
    handle_t h;
    
    if(r) {
        LOGE("___ rtcx read failed!\n");
        return -1;
    }
    
    if(mf->flag.x[mf->date.mon][mf->date.day]==0) {
        
        sprintf(tmp, "%04d/%02d/%02d/ch%d", mf->date.year, mf->date.mon, mf->date.day, ch);
        if(fs_exist(tmp)) {
            h = fs_open(tmp, FS_MODE_RW);
            if(!h) {
                return -1;
            }
            
            //mqtt_pub();
        }
        else {
            memset(&mf->flag, 0, sizeof(mf->flag));
            r = fs_save(tmp, &mf->flag, sizeof(mf->flag));
            if(!h) {
                return -1;
            }
        }
    }
    
    return 0;
}




//////////////////////////////////////////////////////////////////////////
static my_flag_t myFlag;
static U32 sendBuffer[500];
static U32 sendDataID=5654673;
static upload_data_t *pUpData=NULL;
static int up_data_cnt=0;
static int send_mqtt_data(void)
{
    int i,r,tlen=0;
    int size=0;
    node_t node;
    U64 stime_ms;
    U32 step_ms;
    ev_data_t evdata;
    upload_data_t *up=(upload_data_t*)sendBuffer;    
    
    if(list_get(listBuffer.send, &node, 0)==0) {
        int times,evlen,cnt=0;
        ch_data_t *pch=(ch_data_t*)node.buf;
        adc_para_t *para=&allPara.usr.ch[pch->ch];
        F32 *ev=pch->data+pch->dlen/4;
        
        evlen = 4*para->n_ev;
        times = pch->evlen/evlen;
        
        step_ms = (1000*para->evCalcLen)/para->smpFreq;
        stime_ms = pch->time;
        
        up->ch = pch->ch;
        up->ss = up->ch;        //暂时用通道代替sensorID
        up->sig = 9.9F;
        up->cnt = times;
        up->id = sendDataID;
        
        for(i=0; i<up->cnt; i++) {
            evdata.rms = ev[0];
            evdata.amp = ev[1];
            evdata.asl = ev[2];
            evdata.pwr = ev[3];
            evdata.time = stime_ms+i*step_ms;
            memcpy(&up->ev[i], &evdata, sizeof(ev_data_t));
            
            ev += para->n_ev;
        }
        
        LOGD("___ time: %lld, cnt: %d\n", stime_ms, times);
#ifdef PROD_V2
        r = mqtt_pub_str(up);
#else
        r = mqtt_pub_raw(up);
#endif
        //data_save_csv(pch);
    
        sendDataID++;
        
        if(r==0) list_remove(listBuffer.send, 0);
        
        size = list_size(listBuffer.send);
    }
    
    return size;
}

#define TIMER_INTERVAL          300
#define SEND_INTERVAL           (60*1000)
static U32 total_time=0;
void task_comm_send_fn(void *arg)
{
    int i,r,cnt=0;
    int  err=0;
    evt_t e;
    date_time_t dt;
    node_t node;
    
    myFlag.date = allPara.sys.stat.dt.date;
    
    //pUpData = eMalloc(sizeof(upload_data_t)+sizeof(ev_data_t)*EV_SEND_CNT_THRESHOLD);
    
    
    task_tmr_start(TASK_COMM_SEND, tmr_trig_callback, NULL, TIMER_INTERVAL, TIME_INFINITE);
    
    while(1) {
        r = task_recv(TASK_COMM_SEND, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                                
                case EVT_COMM:
                {
                    //tmr_trig_callback(NULL);
                }
                break;
                
                case EVT_TIMER:
                {
                    //检查哪些数据没有发出去，读取后发出去
                    
                    if(mqtt_is_connected()) {
                        int size = send_mqtt_data();
                        
                        //LOGD("___ send list %d\n", size);
                        total_time += TIMER_INTERVAL;
                        if(total_time>SEND_INTERVAL) {
                            total_time = 0;
                            
                            if(size==0) {
                                api_cap_start();
                            }
                        }
                    }
                }
                break;
            }
        }
        
        task_yield();
    }
}
#endif


