#include <math.h>
#include "dsp.h"
#include "log.h"
#include "protocol.h"

static F32 ev_ave_calc(F32 *data, U32 cnt)
{
    U32 i;
    F32 v=0.0F;
    
    for(i=0; i<cnt; i++) {
        v += fabs(data[i]);
    }
    
    return (v/cnt);
}
static F32 ev_rms_calc(F32 *data, U32 cnt)
{
    U32 i;
    F32 v=0.0f;
    
    for(i=0; i<cnt; i++) {
        v += data[i]*data[i];
    }
    v /= cnt;
    
    return (F32)sqrt(v);
}
static F32 ev_asl_calc(F32 *data, U32 cnt)
{
    U32 i;
    F32 v=0.0F;
    
    for(i=0; i<cnt; i++) {
        v += fabs(data[i]);
    }
    v /= cnt;
    
    return (F32)((log10(v)+3)*20);      //dB
}

static F32 ev_ene_calc(F32 *data, U32 cnt)
{
    U32 i;
    F32 rms;
    
    rms = ev_rms_calc(data, cnt);
    
    return ((rms*rms)/50)*1000000;                //pW
}


static F32 ev_min_calc(F32 *data, U32 cnt)
{
    U32 i;
    F32 v=data[0];
    
    for(i=1; i<cnt; i++) {
        if(data[i]<v) {
            v = data[i];
        }
    }
    
    return v;
}
static F32 ev_max_calc(F32 *data, U32 cnt)
{
    U32 i;
    F32 v=data[0];
    
    for(i=1; i<cnt; i++) {
        if(data[i]>v) {
            v = data[i];
        }
    }
    
    return v;
}

static F32 ev_amp_calc(F32 *data, U32 cnt)
{
    U32 i;
    F32 max=fabs(data[0]);
    
    for(i=1; i<cnt; i++) {
        if(fabs(data[i])>max) {
            max = fabs(data[i]);
        }
    }
    
    return (F32)((log10(max)+3) * 20);      //unit: dB
}



int dsp_ev_calc(U8 tp, F32 *data, U32 cnt, U32 freq, F32 *result)
{
    F32 v=0.0F;
    
    if(tp>=EV_NUM || !data || !cnt || !result) {
        return -1;
    }
    
    switch(tp) {
        case EV_AVE: v = ev_ave_calc(data, cnt); break;
        case EV_RMS: v = ev_rms_calc(data, cnt); break;
        case EV_ASL: v = ev_asl_calc(data, cnt); break;
        case EV_ENE: v = ev_ene_calc(data, cnt); break;
        case EV_MIN: v = ev_min_calc(data, cnt); break;
        case EV_MAX: v = ev_max_calc(data, cnt); break;
        case EV_AMP: v = ev_amp_calc(data, cnt); break;
            
        default: return -1;
    }
    *result = v;
    
    return 0;
}




static void fl_fir_calc(F32 *data, U32 cnt)
{
    
}
static void fl_iir_calc(F32 *data, U32 cnt)
{
    
}
static void fl_fft_calc(F32 *data, U32 cnt)
{
    
}
int dsp_fl_calc(U8 tp, F32 *data, U32 cnt)
{
    if(tp>=FL_NUM || !data || !cnt) {
        return -1;
    }
    
    switch(tp) {
        case FL_FIR: fl_fir_calc(data, cnt); break;
        case FL_IIR: fl_iir_calc(data, cnt); break;
        case FL_FFT: fl_fft_calc(data, cnt); break;
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
        dsp_ev_calc(i, data, cnt, 400000, &ev[i]);
        
        LOGD("___[%s]: %.3f\n", str[i], ev[i]);
    }
}


