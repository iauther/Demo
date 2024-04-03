#include <math.h>
#include "dsp.h"
#include "log.h"
#include "protocol.h"
#ifndef _WIN32
//#include "arm_math.h"
#include "arm_const_structs.h"
#endif

static int ev_ave_calc(F32 *data, int cnt, F32 *ev)
{
    int i;
    F32 v=0.0F;
    
    for(i=0; i<cnt; i++) {
        v += fabs(data[i]);
    }
    *ev = v/cnt;
    
    return 0;
}
static int ev_rms_calc(F32 *data, int cnt, F32 *ev)
{
    int i,j=0;
    F32 v=0.0f;
    
    for(i=0; i<cnt; i++) {
        v += data[i]*data[i];
    }
    v /= cnt;
    *ev = (F32)sqrt(v);
    
    return 0;
}
static int ev_asl_calc(F32 *data, int cnt, F32 *ev)
{
    int i;
    F32 v=0.0f;
    
    for(i=0; i<cnt; i++) {
        v += fabs(data[i]);
    }
    v /= cnt;
    *ev = (F32)((log10(v)+3)*20);
    
    return  0;                                      //dB
}

static int ev_ene_calc(F32 *data, int cnt, F32 *ev)
{
    int r;
    F32 rms;
    
    r = ev_rms_calc(data, cnt, &rms);
    *ev = ((rms*rms)/50)*1000000;
    
    return r;                                       //pW
}


static int ev_min_calc(F32 *data, int cnt, F32 *ev)
{
    int i,j=0;
    F32 v=data[0];
    
    for(i=1; i<cnt; i++) {
        if(data[i]<v) {
            v = data[i];
            j = i;
        }
    }
    *ev = v;
    
    return j;
}
static int ev_max_calc(F32 *data, int cnt, F32 *ev)
{
    int i,j=0;
    F32 v=data[0];
    
    for(i=1; i<cnt; i++) {
        if(data[i]>v) {
            v = data[i];
            j = i;
        }
    }
    *ev = v;
    
    return j;
}

static int ev_amp_calc(F32 *data, int cnt, F32 *ev)
{
    int i,j=0;
    F32 max=fabs(data[0]);
    
    for(i=1; i<cnt; i++) {
        if(fabs(data[i])>max) {
            max = fabs(data[i]);
            j = i;
        }
    }
    *ev = (F32)((log10(max)+3) * 20);   //unit: dB
    
    return j;
}



int dsp_ev_calc(U8 tp, F32 *data, int cnt, F32 *ev)
{
    int r;
    
    if(tp>=EV_NUM || !data || !cnt || !ev) {
        return -1;
    }
    
    switch(tp) {
        case EV_AVE: r = ev_ave_calc(data, cnt, ev); break;
        case EV_RMS: r = ev_rms_calc(data, cnt, ev); break;
        case EV_ASL: r = ev_asl_calc(data, cnt, ev); break;
        case EV_ENE: r = ev_ene_calc(data, cnt, ev); break;
        case EV_MIN: r = ev_min_calc(data, cnt, ev); break;
        case EV_MAX: r = ev_max_calc(data, cnt, ev); break;
        case EV_AMP: r = ev_amp_calc(data, cnt, ev); break;
            
        default: return -1;
    }
    
    return r;
}




static void fl_fir_calc(F32 *data, int cnt, U32 sfreq)
{
    //arm_fir_init_f32();
    //arm_fir_f32
    
}
static void fl_iir_calc(F32 *data, int cnt, U32 sfreq)
{
    //arm_iir_f32();
    //arm_iir_f32_process();
}
static void fl_fft_calc(F32 *data, int cnt, U32 sfreq)
{
    int i;
    U32 idx;
    F32 max,freq ;// 用于存放计算结果的频率变量
    int xcnt=4096;//cnt;
    int fftlen=xcnt*2*sizeof(F32);
#ifndef _WIN32
    F32 *pout=malloc(fftlen);
    arm_status r;
    
    if(!pout) {
        return;
    }

#if 1
    arm_rfft_fast_instance_f32 h;
    
    r = arm_rfft_fast_init_f32(&h, xcnt);
    arm_rfft_fast_f32(&h, data, pout, 0);    //FFT正变换
    arm_max_f32(pout, fftlen, &max, &idx);
    freq = (F32)idx*sfreq/xcnt;               // 根据最大值的索引计算信号的频率
#else
    arm_cfft_radix2_instance_f32 h;
    
    arm_cfft_radix2_init_f32(&h, xcnt, 0, 1);
    arm_cfft_radix2_f32(&h, data);
    //arm_cfft_f32(&h, data, 0, 1);// 执行 FFT 计算
    arm_cmplx_mag_f32(data, pout, xcnt);
#endif
    
    free(pout);
#endif

}
int dsp_fl_calc(U8 tp, F32 *data, int cnt, U32 sfreq)
{
    if(tp>=FL_NUM || !data || !cnt) {
        return -1;
    }
    
    switch(tp) {
        case FL_FIR: fl_fir_calc(data, cnt, sfreq); break;
        case FL_IIR: fl_iir_calc(data, cnt, sfreq); break;
        case FL_FFT: fl_fft_calc(data, cnt, sfreq); break;
        default: return -1;
    }
    
    return 0;
}


void dsp_test(F32 *data, int cnt)
{
    int i;
    F32 ev[EV_NUM];
    const char* str[EV_NUM] = { "rms","amp","asl","ene","ave","min","max" };
    
    for(i=0; i<EV_NUM; i++) {
        dsp_ev_calc(i, data, cnt, &ev[i]);
        
        LOGD("___[%s]: %.3f\n", str[i], ev[i]);
    }
}


