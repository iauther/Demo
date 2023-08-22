#include "task.h"
#include "dal_delay.h"
#include "log.h"
#include "list.h"
#include "cfg.h"
#include "ads9120.h"
#include "mem.h"
#include "rtc.h"
#include "paras.h"
#include "fs.h"
#include "power.h"


#ifdef OS_KERNEL


list_buf_t listBuffer;
static void lbuf_init(U32 points)
{
    int i;
    list_cfg_t lc;
    list_buf_t *lb=&listBuffer;
    adc_para_t *pch=allPara.usr.ch;    
    
    //为采集任务申请临时内存
    lb->cap.blen = points*4*2+32;
    lb->cap.buf = eMalloc(lb->cap.blen); 
    
    //为计算任务申请临时内存
    lb->prc.blen = points*4*2+32;
    lb->prc.buf = eMalloc(lb->prc.blen); 
    
    lc.max = 10;
    lc.mode = MODE_FIFO;
    for(i=0; i<CH_MAX; i++) {
        
        if(lb->ori[i]) {
            list_free(lb->ori[i]);
            lb->ori[i]=NULL;
        }
        if(lb->cov[i]) {
            list_free(lb->cov[i]);
            lb->cov[i]=NULL;
        }
        
        if(pch[i].ch<CH_MAX) {
            lb->ori[i] = list_init(&lc);
            lb->cov[i] = list_init(&lc);
        }
    }
    
    //lc.max = 10;
    lb->send = list_init(&lc);
}



static U32 adsLength=0;
static inline U32 get_totalLen(U8 ch)
{
    return allPara.usr.ch[ch].smpPoints*4;
}

#if 0  //将数据拷贝移到任务进行，防止锁影响数据采集
static void ads_data_cb_proc(F32 *data, int cnt)
{
    int r,len=cnt*4,xlen=0;
    int tlen=get_totalLen(CH_0);
    U8 *pbuf=listBuffer.cap.buf;
    U64 time=rtcx_get_timestamp();

    if(adsLength<tlen)
    {
        xlen += sizeof(time);
        memcpy(pbuf, &time, xlen);
        
        if(adsLength+len>tlen) {        //截断数据，只保留用户需要的长度
            len = tlen-adsLength;
        }
        
        memcpy(pbuf+xlen, data, len);
        xlen += len;
        
        r = list_append(listBuffer.ori[CH_0], pbuf, xlen);
        if(r==0) {
            adsLength += len;
            //LOGD("___ori datalen: %d, %dms\n", len, dal_get_tick_ms());
            LOGD("___add to ori ok, %dms\n", dal_get_tick());
        }
        else {
            LOGD("___add to ori failed\n");
        }
    }
    else {
        ads9120_enable(0);
    }
}
#endif

static void ads_data_task_proc(void *data, int length)
{
    int r,xlen=0;
    int dlen,tlen=get_totalLen(CH_0);

    if(adsLength<tlen)
    {
        if(adsLength+dlen>tlen) {        //截断数据，只保留用户需要的长度
            dlen = tlen-adsLength;
        }
        else {
            dlen=length-sizeof(U64);
        }
        
        r = list_append(listBuffer.ori[CH_0], data, dlen+sizeof(U64));
        if(r==0) {
            adsLength += dlen;
            //LOGD("___add to ori ok, %dms\n", dal_get_tick());
        }
        else {
            LOGD("___add to ori failed\n");
        }
    }
    else {
        ads9120_enable(0);
    }
}

static U64 get_start_time(U8 ch, U32 cnt)
{
    U64 time=rtcx_get_timestamp();
    adc_para_t *para=&allPara.usr.ch[ch];
    U32 t_ms=(1000*(cnt-1))/para->smpFreq;
    
    return (time-t_ms);
    
}
static void data_convert(U8 ch, F32 *f, U16 *u, U32 cnt)
{
    U32 i;
    coef_t *coef=&allPara.usr.ch[ch].coef;
    
    for(i=0; i<cnt; i++) {
        f[i] = VOLT(u[i])*coef->a+coef->b;
    }
}
static void ads_data_callback(U16 *data, U32 cnt)
{
    int r;
    U64 stime=get_start_time(CH_0, cnt);
    U8 *pbuf=listBuffer.cap.buf;
    U32 buflen=listBuffer.cap.blen;
    U32 tlen=sizeof(stime)+cnt*4;
    
    if(tlen<buflen) {
        F32 *f32=(F32*)(pbuf+sizeof(U64));
        node_t node={pbuf, buflen, tlen};
        
        memcpy(pbuf, &stime, sizeof(stime));          //数据前带上时间戳
        data_convert(CH_0, f32, data, cnt);
        
        r = task_post(TASK_DATA_CAP, NULL, EVT_ADS, 0, &node, sizeof(node));
        
        //LOGD("__1__%dms\n", dal_get_tick());
    }
}
static void vib_data_callback(U8 *data, U32 len)
{
    node_t node={data,len,len};

    task_post(TASK_DATA_CAP, NULL, EVT_ADC, 0, &node, sizeof(node));
}

static int ads_init(U32 freq)
{
    int r,len;
    U32 points;
    ads9120_cfg_t ac;
    
    adsLength = 0;
    points = (freq/1000)*10;       //10ms
    
    lbuf_init(points);
    
    len = points*sizeof(U16)*2;         //*2表示双buffer
    ac.buf.rx.buf  = (U8*)eMalloc(len);
    ac.buf.rx.blen = len;
    
    len = points*sizeof(U16);
    ac.buf.ox.buf  = (U8*)eMalloc(len);
    ac.buf.ox.blen = len;
    
    ac.freq = freq;
    ac.callback = ads_data_callback;
    
    r = ads9120_init(&ac);
    
    //ads9120_enable(1);
    
    return r;
}
static int vib_init(void)
{
    return 0;
}
/////////////////////////////////////////////////

int api_cap_start(void)
{
    if(ads9120_is_enabled()) {
        return -1;
    }

    return task_trig(TASK_DATA_CAP, EVT_CAP_START);
}
int api_cap_stop(void)
{
    if(!ads9120_is_enabled()) {
        return -1;
    }
    
    return task_trig(TASK_DATA_CAP, EVT_CAP_STOP);
}


///////////////////////////////////////////

static void cap_config(void)
{
    U32 sFreq=allPara.usr.ch[CH_0].smpFreq;
    
    ads_init(sFreq);
    vib_init();
}

static void cap_tmr_callback(void *arg)
{
    //task_post(TASK_DATA_CAP, NULL, EVT_TIMER, 0, NULL, 0);
    task_trig(TASK_DATA_CAP, EVT_TIMER);
}

void task_data_cap_fn(void *arg)
{
    int i,r,r1;
    U8  err;
    evt_t e;    
    
    LOGD("_____ task data cap running\n");
    cap_config();
    
    //task_tmr_start(TASK_DATA_CAP, cap_tmr_callback, NULL, 5, TIME_INFINITE);
    
    while(1) {
        r = task_recv(TASK_DATA_CAP, &e, sizeof(e));
        if(r) {
            continue;
        }
        
        switch(e.evt) {
            
            case EVT_ADC:
            {
                
            }
            break;
            
            case EVT_ADS:
            {
                node_t *nd=(node_t*)e.data;
                
                //LOGD("__2__%dms\n", dal_get_tick());
                ads_data_task_proc(nd->buf, nd->dlen);
            }
            break;
            
            case EVT_CAP_START:
            {
                adsLength = 0;
                ads9120_enable(1);
            }
            break;
            
            case EVT_CAP_STOP:
            {
                ads9120_enable(0);
                adsLength = 0;
            }
            break;
            
            case EVT_TIMER:
            {
                static int cct=0;
                LOGD("___ %d\n", cct++);
            }
            break;
        }
        
        task_yield();
    }
}
#endif














