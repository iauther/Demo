#ifndef __DSP_Hx__
#define __DSP_Hx__

#include "types.h"

int dsp_ev_calc(U8 tp, F32 *data, U32 cnt, U32 freq, F32 *result);
int dsp_fl_calc(U8 tp, F32 *data, U32 cnt);
void dsp_test(F32* data, int cnt);

#endif

