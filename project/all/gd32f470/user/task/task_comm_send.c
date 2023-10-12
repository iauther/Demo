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
    
    //maxCnt = dayMin/allPara.usr.smp.smp_period;
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
    
    //maxCnt = dayMin/allPara.usr.smp.smp_period;
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
static int data_upload(U8 to)
{
    char tmp[40];
    int r,i,j,size=0,grps;
    list_node_t *lnode=NULL;
    handle_t list=taskBuffer.send;
    
    if(list_take(list, &lnode, 0)==0) {
        
        int data_len=lnode->data.dlen;
        char *data_ptr=(char*)lnode->data.buf;
        const char* ev_str[EV_NUM] = { "rms","amp","asl","ene","ave","min","max" };
        
        ch_data_t *p_ch=(ch_data_t*)lnode->data.buf;
        ch_para_t *para=paras_get_ch_para(p_ch->ch);
        ev_data_t *ev=(ev_data_t*)((U8*)(p_ch->data)+p_ch->wavlen);

        if(para->savwav) {      //如果需要保存原始波形，则分发数据到nvm任务进行数据保存
            //
        }
        
        if(!para->upwav) {      //不上传wav数据时，将wav数据剥掉
            p_ch->wavlen = 0;
            memcpy(p_ch->data, ev, p_ch->evlen);
            
            data_len = sizeof(ch_para_t)+p_ch->evlen;
        }
				
        r = comm_send_data(tasksHandle.hconn, &to, TYPE_CAP, 0, data_ptr, data_len);
        if(r==0) {              //发送成功，需要置位发送成功记录标志
            //set send data ok flag
        }
        
        list_back(list, lnode);
    }
    
    return r;
}



static void send_tmr_callback(void *arg)
{
    task_post(TASK_COMM_SEND, NULL, EVT_TIMER, 0, NULL, 0);
}


void task_comm_send_fn(void *arg)
{
    int r;
    evt_t e;
    handle_t htmr;
    myFlag.date = allPara.sys.stat.dt.date;
    htmr = task_timer_init(send_tmr_callback, NULL, 1000, -1);
    if(htmr) {
        task_timer_start(htmr);
    }
    
    while(1) {
        r = task_recv(TASK_COMM_SEND, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                case EVT_DATA:
                case EVT_TIMER:
                {
                    U8 to=allPara.sys.para.sett.datato;
                    
                    data_upload(to);
                }
                break;
            }
        }
        
        osDelay(1);
    }
}
#endif


