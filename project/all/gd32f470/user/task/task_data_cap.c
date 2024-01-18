#include "dal_delay.h"
#include "dal_adc.h"
#include "log.h"
#include "list.h"
#include "cfg.h"
#include "ads9120.h"
#include "mem.h"
#include "rtc.h"
#include "paras.h"
#include "dac.h"
#include "dsp.h"
#include "power.h"
#include "hal_at_impl.h"


dac_param_t dac_param;
#ifdef OS_KERNEL

#include "task.h"

task_buf_t taskBuffer;
/*
static handle_t htmr[CH_MAX]={NULL};
static void tmr_fn(handle_t h, void *arg)
{
    U8 ch=*(U8*)arg;
    
    dal_timer_en(h, 0);
    api_cap_start(ch, STAT_CAP);
}
static void tmr_init(void)
{
    int i;
    U8  tm[CH_MAX]={TIMER_5,TIMER_6};
    dal_timer_cfg_t cfg;
    ch_para_t *para;
    
    cfg.times = 1;
    cfg.callback = tmr_fn;
    for(i=0; i<CH_MAX; i++) {
        para = paras_get_ch_para(i);
        if(para->smpInterval>0) {
            cfg.timer = tm[i];
            cfg.arg = &para->ch;
            cfg.freq = (1000000/para->smpInterval)-1;
            htmr[i] = dal_timer_init(&cfg);
        }
        else {
            htmr[i] = NULL;
        }
    }
    
}
static void tmr_en(U8 ch, U8 on)
{
    dal_timer_en(htmr[ch], on);
}
*/





static U64 get_start_time(U8 ch, U32 cnt)
{
    U64 time=rtc2_get_timestamp_ms();
    ch_para_t *para=paras_get_ch_para(ch);
    U32 t_ms=(1000*(cnt-1))/para->smpFreq;
    
    //LOGD("____ time: %lld, %lld\n", time, time-t_ms);
    return (time-t_ms);
}


static void volt_convert(U8 ch, F32 *f, U16 *u, U32 cnt)
{
    U32 i;
    coef_t *coef=&paras_get_ch_para(ch)->coef;
    
    for(i=0; i<cnt; i++) {
        f[i] = (MVOLT(u[i])-coef->b)/coef->a;
    }
}
static void ori_rms_print(U8 ch, U16 *u, U32 cnt)
{
    U32 i;
    F32 rms,*p=malloc(sizeof(F32)*cnt);
    
    if(p) {
        for(i=0; i<cnt; i++) {
            p[i] = MVOLT(u[i]);
        }
        
        dsp_ev_calc(EV_RMS, p, cnt, &rms);
        LOGD("____ ch-%d ori rms: %f\n", ch, rms);
        
        free(p);
    }
}


static coef_t average_coef(coef_t *x, int cnt)
{
    int i;
    coef_t t={0.0f,0.0f};
    
    for(i=0; i<cnt; i++) {
        t.a += x[i].a;
        t.b += x[i].b;
    }
    t.a /= cnt;
    t.b /= cnt;
    
    return t;
}
static F32 average_f32(F32 *x, int cnt)
{
    int i;
    F32 t=0.0f;
    
    for(i=0; i<cnt; i++) {
        t += x[i];
    }
    t /= cnt;
    
    return t;
}

#define CALI_CNT   100
static int cali_calc(U8 ch, F32 *f, U16 *u, U32 cnt)
{
    int r=0;
    U32 i;
    static F32 rms2[CALI_MAX][CALI_CNT];
    static coef_t coef[CALI_CNT];
    F32 rms,x=0.0f,y=0.0f;
    ch_para_t *pch=paras_get_ch_para(ch);
    cali_t *cali=paras_get_cali(ch);

    if(cali->sig.seq>cali->sig.max || cali->cnt>CALI_CNT) {
        return -1;
    }
        
    for(i=0; i<cnt; i++) {
        f[i] = MVOLT(u[i]); 
        y += f[i];
    }
    
    dsp_ev_calc(EV_RMS, f, cnt, &rms);
    
    if(cali->sig.max==1) {      //1次校准
        
        if(cali->cnt<CALI_CNT) {
            y /= cnt;
            
            coef[cali->cnt].a = rms/cali->rms[0].in;
            coef[cali->cnt].b = cali->sig.bias - (coef[cali->cnt].a*y);
        }
        else {
            pch->coef = average_coef(coef, CALI_CNT);
            
            LOGD("____cali: coef.a: %f, coef.b: %f\n", pch->coef.a, pch->coef.b);
            r = 1;
        }
    }
    else { //多次校准
        if(cali->cnt<CALI_CNT) {
            rms2[cali->sig.seq-1][cali->cnt] = rms;
        }
        else if (cali->cnt==CALI_CNT){
            
            //LOGD("______ rms: %f\n", rms);
            if(cali->sig.seq<cali->sig.max) {
                LOGD("--- cali seq %d finished, input %d cali para please\n", cali->sig.seq, cali->sig.seq+1);
            }
            else {
                int xcnt;
                F32 a=0.0f,b=0.0f,x;

#if 0//def USE_COEF_B
                for(i=0; i<cali->sig.max; i++) {
                    cali->rms[i].out = average_f32(rms2[i], CALI_CNT);
                }
                
                xcnt = cali->sig.max/2;
                for(i=0; i<xcnt; i++) {
                    a += (cali->rms[i*2+1].out-cali->rms[i*2].out)/(cali->rms[i*2+1].in-cali->rms[i*2].in);
                    b += cali->rms[i*2].out - a*cali->rms[i*2].in;
                }
                
                pcs->coef.a = a/xcnt;
                pcs->coef.b = b/xcnt;
#else
                for(i=0; i<cali->sig.max; i++) {
                    cali->rms[i].out = average_f32(rms2[i], CALI_CNT);
                    x = cali->rms[i].out/cali->rms[i].in;
                    a += x;
                    LOGD("___ %d, rms.in: %f, rms.out: %f, coef.a: %f\n", i, cali->rms[i].in, cali->rms[i].out, x);
                }
                
                pch->coef.a = a/cali->sig.max;
                pch->coef.b = 0;
#endif
                LOGD("____cali: coef.a: %f, coef.b: %f\n", pch->coef.a, pch->coef.b);
                r = 1;
            }
        }
    }
    cali->cnt++;
    
    return r;
}



static void ads_data_callback(U16 *data, U32 cnt)
{
    node_t node={0, data, cnt*2, cnt*2};
    task_post(TASK_DATA_CAP, NULL, EVT_ADS, 0, &node, sizeof(node));
}
static void vib_data_callback(U16 *data, U32 cnt)
{
    node_t node={0, data,cnt*2,cnt*2};
    task_post(TASK_DATA_CAP, NULL, EVT_ADC, 0, &node, sizeof(node));
}


static void ads_cali_proc(U8 ch, raw_data_t *raw, U16 *data, int cnt)
{
    int r;
    cali_t *cali=paras_get_cali(ch); 
        
    r = cali_calc(ch, raw->data, data, cnt);
    if(r==1) {
        task_trig(TASK_NVM, EVT_SAVE);
        api_cap_stop(ch);
    }
}
static void ads_cap_proc(U8 ch, raw_data_t *raw, U16 *data, int cnt)
{
    int r;
    task_buf_t *tb=&taskBuffer;
    handle_t list=tb->var[ch].raw;
    U64 stime=get_start_time(ch, cnt);
    U32 buflen=tb->var[ch].cap.blen;
    U32 tlen=cnt*sizeof(raw_t)+sizeof(raw_data_t);
    ch_var_t *cv=&tb->var[ch];
    ch_para_t *para=paras_get_ch_para(ch);
    
    int smp_len=para->smpPoints*sizeof(U16);
    int t_slen=(para->smpInterval/1000000)*(para->smpFreq*sizeof(U16));
    int data_len=cnt*2;
    
    U16 *real_data=data;
    int   real_len=data_len;
    
    dac_data_fill(data, cnt);
    
    if(para->smpMode==SMP_MODE_PERIOD) {
            
        if(cv->times>=para->smpTimes) {
            return;
        }
        
        //达到设定的单次采样长度后，开始数据跳帧
        if(cv->rlen>=smp_len) {
            
            //超过设定的采集间隔长度时，按设定值截取
            if(cv->slen+data_len>=t_slen) {
                int x=t_slen-cv->slen;
                real_len -= x;
                real_data += x/sizeof(U16);
                
                cv->rlen = 0;
                cv->slen = 0;
            }
            else {
                cv->slen += data_len;
                real_len = 0;       //skip data
            }
        }
        else {
            
            if(cv->rlen+data_len>=smp_len) {
                real_len = smp_len-cv->rlen;
                cv->rlen = smp_len;
            }
            else {
                cv->rlen += data_len;
            }
        } 
    }

    if(real_len>0) {
        raw->ch = ch;
        raw->time = stime;
        raw->cnt  = real_len/sizeof(U16);
        
        //ori_rms_print(ch, real_data, xcnt);     //just for debug        
        volt_convert(raw->ch, raw->data, real_data, raw->cnt);
        
        //LOGD("_____ ch[%d]  rlen: %d, smplen: %d, skplen: %d, t_skplen: %d, times: %d, real_len: %d\n", ch, cv->rlen, smp_len, cv->slen, t_slen, cv->times, real_len);
        tlen = raw->cnt*sizeof(raw_t)+sizeof(raw_data_t);
        r = api_data_proc_send(ch, raw, tlen);
        
        if(para->smpMode==SMP_MODE_PERIOD) {
            if(cv->rlen+data_len>=smp_len) {
                cv->times++;
            }
            
            //周期采样时，达到设定的次数则停止采样
            if(para->smpMode==SMP_MODE_PERIOD && cv->times>=para->smpTimes) {
                cv->rlen = 0; cv->slen = 0;
                api_cap_power(raw->ch, 0);
                paras_set_finished(raw->ch, 1);
            }
        }
        //LOGD("__1__%dms\n", dal_get_tick());
    }
}

static int ads_data_proc(U8 ch, node_t *nd)
{
    int cnt=nd->dlen/2;
    U8 mode=paras_get_mode();
    task_buf_t *tb=&taskBuffer;
    raw_data_t *raw=(raw_data_t*)tb->var[ch].cap.buf;
    
    extern volatile void* p_dma;
    if(p_dma != nd->buf) {
        LOGE("+\n");
        return 1;
    }
    
    if(mode==MODE_CALI) {
        ads_cali_proc(ch, raw, (U16*)nd->buf, cnt);
    }
    else {
        ads_cap_proc(ch, raw, (U16*)nd->buf, cnt);
    }
    
    return 0;
}


static int ads_init(void)
{
    int r,len;
    U32 points;
    dac_param_t dp;
    ads9120_cfg_t ac;
    ch_para_t *para=paras_get_ch_para(CH_0);
    
    points = (para->smpFreq/1000)*SAMPLE_INT_INTERVAL;            //10ms
    
    len = points*sizeof(U16)*2;         //*2表示双buffer
    ac.buf.rx.buf  = (U8*)eMalloc(len);
    ac.buf.rx.blen = len;
    
    ac.freq = para->smpFreq;
    ac.callback = ads_data_callback;
    
    r = ads9120_init(&ac);
    
    dp.freq = para->smpFreq;
    dp.enable = 0; //allPara.usr.dac.enable;
    dp.fdiv   = allPara.usr.dac.fdiv;
    dp.buf.blen = points*sizeof(U16)/dp.fdiv;
    dp.buf.buf  = eMalloc(dp.buf.blen);
    dac_init(&dp);

    return r;
}
static int vib_init(void)
{
    dal_adc_cfg_t dc;
    
    //dc.callback = vib_data_callback;
    dc.callback = NULL;
    dc.freq = 1000;
    dal_adc_init(&dc);
    
    return 0;
}
/////////////////////////////////////////////////
int api_cap_start(U8 ch)
{
    ch_para_t *para=paras_get_ch_para(ch);
    
    if(!para->enable) {
        return -1;
    }
    
    if(paras_get_state(ch)==STAT_RUN) {
        return 0;
    }
    
    if(ch==CH_0) {
        ads9120_enable(1);
        dac_start();
    }
    else if(ch==CH_1) {
        
    }
    paras_set_state(ch, STAT_RUN);
    paras_set_finished(ch, 0);
    
    return 0;
}
int api_cap_stop(U8 ch)
{
    int i;
    task_buf_t *tb=&taskBuffer;
    ch_para_t *para=paras_get_ch_para(ch);
    
    if(!para->enable) {
        return -1;
    }
    
    if(ch==CH_0) {
        ads9120_enable(0);
        dac_stop();
    }
    else if(ch==CH_1) {
        
    }
    tb->var[ch].rlen = 0;
    tb->var[ch].slen = 0;
    tb->var[ch].times = 0;
    tb->var[ch].thr.max = 0.0f;
    tb->var[ch].cap.dlen = 0;
    tb->var[ch].prc.dlen = 0;
    paras_set_state(ch, STAT_STOP);
    
    return 0;
}

int api_cap_start_all(void)
{
    U8 ch;
    
    for(ch=0; ch<CH_MAX; ch++) {
        api_cap_start(ch);
    }
    
    return 0;
}
int api_cap_stop_all(void)
{
    U8 ch;
    
    for(ch=0; ch<CH_MAX; ch++) {
        api_cap_stop(ch);
    }
    
    return 0;
}



int api_cap_power(U8 ch, U8 on)
{
    ch_para_t *pch=paras_get_ch_para(ch);
    
    if(!pch->enable) {
        return -1;
    }
    
    if(ch==CH_0) {
        ads9120_enable(on);
    }
    else {
        //
    }
    
    return 0;
}


///////////////////////////////////////////

static void cap_config(void)
{
    ads_init();
    vib_init();
}
void print_ts(char *s)
{
    static U64 ts=0,ts2;
    
    ts2 = dal_get_tick_ms();
    LOGD("%s tick: %lld\n", s, ts2-ts);
    ts = ts2;
}


void task_data_cap_fn(void *arg)
{
    int i,r,r1;
    U8  err;
    evt_t e;    
    
    LOGD("_____ task data cap running\n");
    cap_config();
    
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
                ads_data_proc(CH_0, (node_t*)e.data);
            }
            break;
        }
        
        //task_yield();
    }
}
#endif














