#ifndef __DSP_Hx__
#define __DSP_Hx__

#include "types.h"

typedef enum {
    EV_RMS=0,
    EV_AMP,
    EV_ASL,
    EV_ENE,
    EV_AVE,         //average
    EV_MIN,
    EV_MAX,
    
    EV_NUM
}EV_TYPE;

typedef enum {
    FLT_FIR=0,
    FLT_IIR,
    FLT_FFT,
}FLT_TYPE;


int dsp_ev_calc(EV_TYPE ev, F32 *data, U32 cnt, U32 freq, F32 *result);
int dsp_flt_calc(FLT_TYPE flt, F32 *data, U32 cnt);


#endif

