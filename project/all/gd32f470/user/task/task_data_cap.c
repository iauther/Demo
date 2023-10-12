#include "task.h"
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


#ifdef OS_KERNEL


static handle_t htmr=NULL;
static U64 cap_time[CH_MAX]={0};
task_buf_t taskBuffer;
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
        
        int once_smp_len=(para->smpFreq/1000)*SAMPLE_INT_INTERVAL*sizeof(raw_t)+32;
        int once_ev_calc_data_len=para->evCalcCnt*sizeof(raw_t);
        
        //计算单次采集能满足ev计算时所需内存长度
        len1 = once_smp_len+para->n_ev*sizeof(ev_data_t)*(once_smp_len/once_ev_calc_data_len)+sizeof(ch_data_t)+32;
        
        //计算特征值计算时需要的内存长度
        len2 = once_ev_calc_data_len+para->n_ev*sizeof(ev_data_t)+sizeof(ch_data_t)+32;
        
        //计算阈值触发时缓存设定要求所需内存长度, (1ms+smpPoints)*2
        len3 = (para->smpPoints+para->smpFreq/1000)*2*sizeof(raw_t)+32;
        
        tb->var[i].prc.blen = MAX3(len1,len2,len3);
        tb->var[i].prc.buf = eMalloc(tb->var[i].prc.blen); 
        tb->var[i].prc.dlen = 0;
    }
    
    //lc.max = 10;
    tb->send = list_init(&lc);
}


static U64 get_start_time(U8 ch, U32 cnt)
{
    U64 time=rtcx_get_timestamp();
    ch_para_t *para=paras_get_ch_para(ch);
    U32 t_ms=(1000*(cnt-1))/para->smpFreq;
    
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

extern U8 para_send_flag;
static void cali_calc_1(U8 ch, F32 *f, U16 *u, U32 cnt)
{
    U32 i;
    F32 rms,x=0.0f,y=0.0f;
    ch_para_t *para=paras_get_ch_para(ch);
    cali_sig_t *sig=&paras_get_cali(ch)->sig;
    
    for(i=0; i<cnt; i++) {
        f[i] = MVOLT(u[i]);
        y += f[i];
    }
    y /= cnt;

    dsp_ev_calc(EV_RMS, f, cnt, para->smpFreq, &rms);
    para->coef.a = rms/sig->rms;
    para->coef.b = sig->bias - (para->coef.a*y);
    
    LOGD("____cali: coef.a: %f, coef.b: %f\n", para->coef.a, para->coef.b);
    
    para_send_flag = 0;
    task_trig(TASK_POLLING, EVT_SAVE);
}


static void cali_calc_2(U8 ch, F32 *f, U16 *u, U32 cnt)
{
    U32 i;
    F32 rms;
    ch_para_t *para=paras_get_ch_para(ch);
    cali_t *cali=paras_get_cali(ch);
    
    for(i=0; i<cnt; i++) {
        f[i] = MVOLT(u[i]);
    }
    dsp_ev_calc(EV_RMS, f, cnt, para->smpFreq, &rms);
    
    cali->rms[cali->cnt-1].out = rms;
    
    if(cali->cnt>=2) {
        para->coef.a = (cali->rms[1].out-cali->rms[0].out)/(cali->rms[1].in-cali->rms[0].in);
        para->coef.b = cali->rms[1].out - para->coef.a*cali->rms[1].in;
        
        LOGD("____cali: coef.a: %f, coef.b: %f\n", para->coef.a, para->coef.b);
        
        para_send_flag = 0;
        task_trig(TASK_POLLING, EVT_SAVE);
        cali->cnt = 0;
    }
}


static void ads_data_callback(U16 *data, U32 cnt)
{
    node_t node={data, cnt*2, cnt*2};
    task_post(TASK_DATA_CAP, NULL, EVT_ADS, 0, &node, sizeof(node));
}
static void vib_data_callback(U16 *data, U32 cnt)
{
    node_t node={data,cnt*2,cnt*2};
    task_post(TASK_DATA_CAP, NULL, EVT_ADC, 0, &node, sizeof(node));
}


#define CALI_THD_CNT            30
static int ads_data_cnt=0;
static int ads_data_proc(node_t *nd)
{
    int i,r,cnt=nd->dlen/2;
    U16 *data=(U16*)nd->buf;
    task_buf_t *tb=&taskBuffer;
    handle_t list=tb->var[CH_0].raw;
    raw_data_t *raw=(raw_data_t*)tb->var[CH_0].cap.buf;
    
    if(ads_data_cnt<CALI_THD_CNT) {
        ads_data_cnt++;
        return -1;
    }
    
    if(paras_get_state()==STAT_CALI) {
        
        cali_t *cali=paras_get_cali(CH_0); 
        
        if(cali->sig.tms==1) {
            cali_calc_1(CH_0, raw->data, data, cnt);
        }
        else {
            cali_calc_2(CH_0, raw->data, data, cnt);
        }
        
        paras_state_restore();
    }
    else {
        U64 stime=get_start_time(CH_0, cnt);
        U32 buflen=tb->var[CH_0].cap.blen;
        U32 tlen=cnt*sizeof(raw_t)+sizeof(raw_data_t);
        
        dac_data_fill(data, cnt);       //output to dac
        
        if(tlen<=buflen) {            
            raw->time = stime;
            volt_convert(CH_0, raw->data, data, cnt);
            
            r = list_append(list, raw, tlen);
            
            //LOGD("__1__%dms\n", dal_get_tick());
        }
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
    dp.points = points;
    dp.enable = allPara.usr.dac.enable;
    dp.fdiv   = allPara.usr.dac.fdiv;
    dac_set(&dp);

#ifdef DEMO_TEST
    api_cap_start(CH_0);
    paras_set_state(STAT_CAP);
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
    if(ch==CH_0) {
        if(ads9120_is_enabled()) {
            return -1;
        }

        ads_data_cnt = 0;
        ads9120_enable(1);
        dac_start();
    }
    else {
        
    }
    
    cap_time[ch] = rtcx_get_timestamp();
    
    return 0;
}
int api_cap_stop(U8 ch)
{
    int i;
    task_buf_t *tb=&taskBuffer;
    
    if(ch==CH_0) {
        if(!ads9120_is_enabled()) {
            return -1;
        }
        
        ads_data_cnt = 0;
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
    
    return 0;
}

//所有通道都停止采集时返回1
int api_cap_stoped(void)
{
    if(ads9120_is_enabled()) {
        return 0;
    }
    
    return 1;
}


int api_cap_power(U8 ch, U8 on)
{
    if(ch==CH_0) {
        ads9120_power(on);
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
                ads_data_proc((node_t*)e.data);
            }
            break;
        }
        
        task_yield();
    }
}
#endif














