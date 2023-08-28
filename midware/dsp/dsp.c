#include "dsp.h"
#include <math.h>

static F32 ev_ave_calc(F32 *data, U32 cnt)
{
    U32 i;
    F32 v=0.0F;
    
    for(i=0; i<cnt; i++) {
        v += data[i];
    }
    
    return (v/cnt);
}
static F32 ev_rms_calc(F32 *data, U32 cnt)
{
    U32 i;
    F64 v=0;
    
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
    v *= 1000000;
    
    return (F32)(log10(v)*20);
}

static F32 ev_ene_calc(F32 *data, U32 cnt, U32 freq)
{
    U32 i;
    F32 s=0.0F;
    F32 tm=(1.0F/freq);
    
    for(i=0; i<cnt-1; i++) {
        s += data[i]*tm;
    }
    
    return s;
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
    
    return max;
}



int dsp_ev_calc(EV_TYPE ev, F32 *data, U32 cnt, U32 freq, F32 *result)
{
    F32 v=0.0F;
    
    if(!data || !cnt || !result) {
        return -1;
    }
    
    switch(ev) {
        case EV_AVE: v = ev_ave_calc(data, cnt); break;
        case EV_RMS: v = ev_rms_calc(data, cnt); break;
        case EV_ASL: v = ev_asl_calc(data, cnt); break;
        case EV_ENE: v = ev_ene_calc(data, cnt, freq); break;
        case EV_MIN: v = ev_min_calc(data, cnt); break;
        case EV_MAX: v = ev_max_calc(data, cnt); break;
        case EV_AMP: v = ev_amp_calc(data, cnt); break;
            
        default: return -1;
    }
    *result = v;
    
    return 0;
}




static void flt_fir_calc(F32 *data, U32 cnt)
{
    
}
static void flt_iir_calc(F32 *data, U32 cnt)
{
    
}
static void flt_fft_calc(F32 *data, U32 cnt)
{
    
}
int dsp_flt_calc(FLT_TYPE flt, F32 *data, U32 cnt)
{
    if(!data || !cnt) {
        return -1;
    }
    
    switch(flt) {
        case FLT_FIR: flt_fir_calc(data, cnt); break;
        case FLT_IIR: flt_iir_calc(data, cnt); break;
        case FLT_FFT: flt_fft_calc(data, cnt); break;
        default: return -1;
    }
    
    return 0;
}

