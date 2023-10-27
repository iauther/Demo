#include "dal_delay.h"
#include "log.h"
#include "list.h"
#include "cfg.h"
#include "ads9120.h"
#include "mem.h"
#include "rtc.h"
#include "paras.h"
#include "dac.h"
#include "power.h"
#include "hw_at_impl.h"


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



static void buf_init()
{
    int i;
    list_cfg_t lc;
    int len1,len2,len3;
    ch_para_t *para;
    task_buf_t *tb=&taskBuffer;
    
    lc.max = 10;
    lc.mode = MODE_FIFO;
    for(i=0; i<CH_MAX; i++) {
        
        if(tb->var[i].raw) {
            list_free(tb->var[i].raw);
            tb->var[i].raw=NULL;
        }
        tb->var[i].raw = list_init(&lc);
        
        tb->var[i].rlen = 0;
        tb->var[i].slen = 0;
        tb->var[i].times = 0;
        tb->var[i].ts[0] = 0;
        tb->var[i].ts[1] = 0;
        
        //为采集任务申请临时内存
        if(tb->var[i].cap.buf) {
            xFree(tb->var[i].cap.buf);
        }
        tb->var[i].cap.blen = paras_get_smp_cnt(i)*sizeof(raw_t)+sizeof(raw_data_t)+32;
        tb->var[i].cap.buf = eMalloc(tb->var[i].cap.blen); 
        
        para = paras_get_ch_para(i);
        
        //为计算任务申请临时内存
        if(tb->var[i].prc.buf) {
            xFree(tb->var[i].prc.buf);
        }
        
        //申请ev内存
        int smp_cnt = (para->smpFreq/1000)*SAMPLE_INT_INTERVAL;
        int ev_grps = smp_cnt/para->evCalcCnt+1;
        int ev_len = sizeof(ev_data_t)+(sizeof(ev_grp_t)+sizeof(ev_val_t)*para->n_ev)*ev_grps;
        tb->var[i].ev.blen = ev_len;
        tb->var[i].ev.buf  = eCalloc(tb->var[i].ev.blen); 
        tb->var[i].ev.dlen = 0;
        
        
        int once_smp_len=(para->smpFreq/1000)*SAMPLE_INT_INTERVAL*sizeof(raw_t)+32;
        int once_ev_calc_data_len=para->evCalcCnt*sizeof(raw_t);
        
        //计算单次采集能满足ev计算时所需内存长度
        len1 = once_smp_len+ev_len+sizeof(ch_data_t)+32;
        
        //计算特征值计算时需要的内存长度
        len2 = once_ev_calc_data_len+ev_len+sizeof(ch_data_t)+32;
        
        //计算阈值触发时缓存设定要求所需内存长度, (1ms+smpPoints)*2
        len3 = (para->smpPoints+para->smpFreq/1000)*2*sizeof(raw_t)+32;
        
        tb->var[i].prc.blen = MAX3(len1,len2,len3);
        tb->var[i].prc.buf  = eCalloc(tb->var[i].prc.blen); 
        tb->var[i].prc.dlen = 0;
    }
    
    tb->file = list_init(&lc);
    
    //lc.max = 10;
    tb->send = list_init(&lc);
}


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
    coef_t *coef=&paras_get_ch_paras(ch)->coef;
    
    for(i=0; i<cnt; i++) {
        f[i] = (MVOLT(u[i])-coef->b)/coef->a;
    }
}

#define CALI_CNT   100
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

static int cali_calc_1(U8 ch, F32 *f, U16 *u, U32 cnt)
{
    U32 i;
    static int cali1_cnt=0;
    static coef_t coef1[CALI_CNT];
    F32 rms,x=0.0f,y=0.0f;
    ch_para_t *pc=paras_get_ch_para(ch);
    ch_paras_t *pcs=paras_get_ch_paras(ch);
    cali_sig_t *sig=&paras_get_cali(ch)->sig;
    
    for(i=0; i<cnt; i++) {
        f[i] = MVOLT(u[i]);
        y += f[i];
    }
    y /= cnt;

    dsp_ev_calc(EV_RMS, f, cnt, pc->smpFreq, &rms);
    coef1[cali1_cnt].a = rms/sig->rms;
    coef1[cali1_cnt].b = sig->bias - (coef1[cali1_cnt].a*y);
    
    cali1_cnt++;
    if(cali1_cnt>=CALI_CNT) {
        
        pcs->coef = average_coef(coef1, CALI_CNT);
        cali1_cnt = 0;
        LOGD("____cali: coef.a: %f, coef.b: %f\n", pcs->coef.a, pcs->coef.b);
        
        return 1;
    }

    return 0;
}


static int cali_calc_2(U8 ch, F32 *f, U16 *u, U32 cnt)
{
    U32 i;
    static F32 rms2[2][CALI_CNT];
    static int cali2_cnt[2]={0,0};
    ch_para_t *pc=paras_get_ch_para(ch);
    ch_paras_t *pcs=paras_get_ch_paras(ch);
    cali_t *cali=paras_get_cali(ch);

    if(cali2_cnt[cali->cnt-1]<CALI_CNT) {
        
        for(i=0; i<cnt; i++) {
            f[i] = MVOLT(u[i]);
        }
        
        dsp_ev_calc(EV_RMS, f, cnt, pc->smpFreq, &rms2[cali->cnt-1][cali2_cnt[cali->cnt-1]]);
        cali2_cnt[cali->cnt-1]++;
    }
    else if(cali2_cnt[cali->cnt-1]==CALI_CNT) {
        if(cali->cnt>=2) {
            
            cali->rms[0].out = average_f32(rms2[0], CALI_CNT);
            cali->rms[1].out = average_f32(rms2[1], CALI_CNT);
            
            pcs->coef.a = (cali->rms[1].out-cali->rms[0].out)/(cali->rms[1].in-cali->rms[0].in);
            pcs->coef.b = cali->rms[1].out - pcs->coef.a*cali->rms[1].in;
            
            LOGD("____cali: coef.a: %f, coef.b: %f\n", pcs->coef.a, pcs->coef.b);
            cali->cnt = 0;
            cali2_cnt[0] = cali2_cnt[1] = 0;
            
            return 1;
        }
        
        LOGD("_____ input the scond cali param and signal!\n");
        cali2_cnt[cali->cnt-1]++;
    }
    
    return 0;
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



static int ads_data_proc(U8 ch, node_t *nd)
{
    int i,r,cnt=nd->dlen/2;
    U8 mode=paras_get_mode();
    ch_para_t *para=paras_get_ch_para(ch);
    task_buf_t *tb=&taskBuffer;
    ch_var_t *cv=&tb->var[ch];
    handle_t list=tb->var[ch].raw;
    raw_data_t *raw=(raw_data_t*)tb->var[ch].cap.buf;
    int smp_len=para->smpPoints*sizeof(raw_t);
    int t_slen=(para->smpInterval/1000000)*(para->smpFreq*sizeof(U16));
    int data_len=nd->dlen;
    
    U16 *real_data=(U16*)nd->buf;
    int   real_len=data_len;
    
    if(mode==MODE_CALI) {
        
        cali_t *cali=paras_get_cali(ch); 
        
        if(cali->sig.tms==1) {
            r = cali_calc_1(ch, raw->data, real_data, cnt);
        }
        else {
            r = cali_calc_2(ch, raw->data, real_data, cnt);
        }
        
        if(r) {
            task_trig(TASK_NVM, EVT_SAVE);
             
            api_cap_stop(ch);
        }
    }
    else {
        U64 stime=get_start_time(ch, cnt);
        U32 buflen=tb->var[ch].cap.blen;
        U32 tlen=cnt*sizeof(raw_t)+sizeof(raw_data_t);
        
        dac_data_fill(real_data, cnt);       //output to dac
        
        
        if(para->smpMode==SMP_PERIOD_MODE) {
            
            //周期采样时，达到设定的次数则停止采样
            if(cv->times>=para->smpTimes) {
                cv->rlen = 0; cv->slen = 0;
                //LOGD("____ api_cap_power off ch: %d \n", ch);
                
                api_cap_power(ch, 0);
                paras_set_finished(ch, 1);
                
                return -1;
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
                    
                    cv->times++;
                }
                else {
                    cv->rlen += data_len;
                }
            } 
        }
        //
        
        if(real_len>0) {
            int xcnt=real_len/4;
            raw->time = stime;
            
            volt_convert(ch, raw->data, real_data, xcnt);
            
            //LOGD("_____ ch[%d]  rlen: %d, smplen: %d, skplen: %d, t_skplen: %d, times: %d, real_len: %d\n", ch, cv->rlen, smp_len, cv->slen, t_slen, cv->times, real_len);
            tlen = real_len+sizeof(raw_data_t);
            r = list_append(list, 0, raw, tlen);
            
            //LOGD("__1__%dms\n", dal_get_tick());
        }
    }
    
    return 0;
}


dac_param_t dac_param;
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
    dp.points = points;
    dp.enable = allPara.usr.dac.enable;
    dp.fdiv   = allPara.usr.dac.fdiv;
    dac_set(&dp);

#ifdef DEMO_TEST
    if(paras_get_mode()!=MODE_CALI) {
        api_cap_start_all();
    }
#endif

    return r;
}
static int vib_init(void)
{
    return 0;
}
/////////////////////////////////////////////////
int api_cap_start(U8 ch)
{
    ch_paras_t *p=paras_get_ch_paras(ch);
    
    if(!p->enable) {
        return -1;
    }
    
    at_hal_power(0);        //启动采集时先关4g模组
    
    if(ch==CH_0) {
        ads9120_enable(1);
        dac_start();
    }
    else {
        
    }
    paras_set_state(ch, STAT_RUN);
    paras_set_finished(ch, 0);
    
    return 0;
}
int api_cap_stop(U8 ch)
{
    int i;
    task_buf_t *tb=&taskBuffer;
    ch_paras_t *p=paras_get_ch_paras(ch);
    
    if(!p->enable) {
        return -1;
    }
    
    if(ch==CH_0) {
        ads9120_enable(0);
        dac_stop();
    }
    else {
        
    }
    tb->var[ch].rlen = 0;
    tb->var[ch].slen = 0;
    tb->var[ch].times = 0;
    tb->var[ch].ts[0] = 0;
    tb->var[ch].ts[1] = 0;
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
    ch_paras_t *p=paras_get_ch_paras(ch);
    
    if(!p->enable) {
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


void task_data_cap_fn(void *arg)
{
    int i,r,r1;
    U8  err;
    evt_t e;    
    
    LOGD("_____ task data cap running\n");
    buf_init();
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
        
        task_yield();
    }
}
#endif














