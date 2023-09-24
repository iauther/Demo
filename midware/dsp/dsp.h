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
    FL_FIR=0,
    FL_IIR,
    FL_FFT,
    
    FL_NUM
}FLT_TYPE;


int dsp_ev_calc(U8 tp, F32 *data, U32 cnt, U32 freq, F32 *result);
int dsp_fl_calc(U8 tp, F32 *data, U32 cnt);


#endif

