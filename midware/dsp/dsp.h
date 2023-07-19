#ifndef __DSP_Hx__
#define __DSP_Hx__

#include "types.h"

typedef enum {
    EV_AVE=0,     //average 
    EV_RMS,
    EV_ASL,
    EV_MIN,
    EV_MAX,
}EV_TYPE;

typedef enum {
    FLT_FIR=0,
    FLT_IIR,
    FLT_FFT,
}FLT_TYPE;


int dsp_ev_calc(EV_TYPE ev, U16 *data, U32 len, F32 *result);
int dsp_flt_calc(FLT_TYPE flt, U16 *data, U32 len);


#endif

