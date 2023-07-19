#include "dsp.h"
#include <math.h>

static F32 ev_ave_calc(U16 *data, U32 len)
{
    U32 i,v=0;
    
    for(i=0; i<len; i++) {
        v += data[i];
    }
    
    return (F32)(v/len);
}
static F32 ev_rms_calc(U16 *data, U32 len)
{
    U32 i,x;
    F64 v=0;
    
    for(i=0; i<len; i++) {
        v += data[i]*data[i];
    }
    v /= len;
    
    return (F32)sqrt(v);
}
static F32 ev_asl_calc(U16 *data, U32 len)
{
    return 0.0F;
}
static F32 ev_min_calc(U16 *data, U32 len)
{
    U32 i;
    F32 v=data[0];
    
    for(i=1; i<len; i++) {
        if(data[i]<v) {
            v = data[i];
        }
    }
    
    return v;
}
static F32 ev_max_calc(U16 *data, U32 len)
{
    U32 i;
    F32 v=data[0];
    
    for(i=1; i<len; i++) {
        if(data[i]>v) {
            v = data[i];
        }
    }
    
    return v;
}



int dsp_ev_calc(EV_TYPE ev, U16 *data, U32 len, F32 *result)
{
    F32 v=0.0F;
    
    if(!data || !len || !result) {
        return -1;
    }
    
    switch(ev) {
        case EV_AVE: v = ev_ave_calc(data, len); break;
        case EV_RMS: v = ev_ave_calc(data, len); break;
        case EV_ASL: v = ev_ave_calc(data, len); break;
        case EV_MIN: v = ev_min_calc(data, len); break;
        case EV_MAX: v = ev_max_calc(data, len); break;
            
        default: return -1;
    }
    *result = v;
    
    return 0;
}




static void flt_fir_calc(U16 *data, U32 len)
{
    
}
static void flt_iir_calc(U16 *data, U32 len)
{
    
}
static void flt_fft_calc(U16 *data, U32 len)
{
    
}
int dsp_flt_calc(FLT_TYPE flt, U16 *data, U32 len)
{
    if(!data || !len) {
        return -1;
    }
    
    switch(flt) {
        case FLT_FIR: flt_fir_calc(data, len); break;
        case FLT_IIR: flt_iir_calc(data, len); break;
        case FLT_FFT: flt_fft_calc(data, len); break;
        default: return -1;
    }
    
    return 0;
}

