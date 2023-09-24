#include "task.h"
#include "list.h"
#include "dal_rtc.h"
#include "dal_delay.h"
#include "fs.h"
#include "rtc.h"
#include "cfg.h"
#include "comm.h"
#include "paras.h"



#define FILE_MAX_LEN        (1024*1024)


#ifdef OS_KERNEL
static int comm_iterator_fn(handle_t l, node_t *data, node_t *node, void *arg, int *act)
{
    handle_t hconn=(handle_t)(*(U32*)node->buf);
    
    comm_send_data(tasksHandle.hconn, "", TYPE_CAP, 0, data->buf, data->dlen);
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
    
    maxCnt = dayMin/allPara.usr.smp.smp_period;
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
        wlen = fs_append(xHandle, pch->data, pch->wavlen, 0);
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
    int dayMin=(24*3600);
    int flen,wlen,maxCnt,maxLen;
    fs_space_t sp;
    
    maxCnt = dayMin/allPara.usr.smp.smp_period;
    maxLen = paras_get_smp_cnt(pch->ch)*4*maxCnt;
    
    r = fs_get_space(SDMMC_MNT_PT, &sp);
    if(r==0 && sp.free<pch->wavlen*10) {
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
        int times=pch->wavlen/onelen;
        int left=pch->wavlen%onelen;
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



extern int mqtt_pub_raw(void *data, int len);
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
static int upload_by_mqtt(void)
{
    int i,r,tlen=0;
    int size=0;
    node_t node;
    handle_t list=taskBuffer.send;
    
    if(!mqtt_is_connected()) {
        return -1;
    }
    
    if(list_get(list, &node, 0)==0) {

        r = mqtt_pub_raw(node.buf, node.dlen);
        //data_save_csv(pch);
        
        if(r==0) list_remove(list, 0);
        
        size = list_size(list);
    }
    
    return size;
}
static int upload_by_self(void)
{
    int r,size=0;
    node_t node;
    handle_t list=taskBuffer.send;
    
    if(list_get(list, &node, 0)==0) {
        ch_data_t *pch=(ch_data_t*)node.buf;
        
        r = comm_send_data(tasksHandle.hconn, "", TYPE_CAP, 0, node.buf, node.dlen);
        if(r==0) list_remove(taskBuffer.send, 0);
        
        size = list_size(list);
    }
    
    return size;
}
static int upload_by_debug(void)
{
    int i,j,size=0,grps;
    node_t node;
    handle_t list=taskBuffer.send;
    
    if(list_take(list, &node, 0)==0) {
        ch_data_t *p_ch=(ch_data_t*)node.buf;
        const char *ev_str[EV_NUM]={"rms","asl","ene","ave","min","max"};

        ch_para_t *para=&allPara.usr.ch[p_ch->ch];
        ev_data_t *ev=(ev_data_t*)((U8*)p_ch->data+p_ch->wavlen);
        
        grps = 1;//pdata->evlen/(para->n_ev*sizeof(ev_data_t));
        for(i=0; i<grps; i++) {
            for(j=0; j<para->n_ev; j++) {
                LOGD("%s: %f\n", ev_str[para->ev[j]], ev[j].data);
            }
            LOGD("\n");
        }
        list_back(list, &node);
        
        size = list_size(list);
    }
    
    return size;
}



//0: mqtt  1: self   2: debug
enum {
    MQTT=0,
    SELF,
    DEBUG,
};
static void upload_data(int upway)
{
    int r=-1,state=paras_get_state();
    
    switch(upway) {
        case MQTT:
        r = upload_by_mqtt();
        break;
        
        case SELF:
        r = upload_by_self();
        break;
        
        case DEBUG:
        r = upload_by_debug();
        break;
    }
}



void task_comm_send_fn(void *arg)
{
    int r;
    evt_t e;

    myFlag.date = allPara.sys.stat.dt.date;
    
    while(1) {
        r = task_recv(TASK_COMM_SEND, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_DATA:
                {
                    upload_data(DEBUG);
                }
                break;
            }
        }
        
        osDelay(1);
    }
}
#endif


