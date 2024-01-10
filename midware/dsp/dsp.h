#ifndef __DSP_Hx__
#define __DSP_Hx__

#include "types.h"

int dsp_ev_calc(U8 tp, F32 *data, int cnt, F32 *ev);
int dsp_fl_calc(U8 tp, F32 *data, int cnt);
void dsp_test(F32* data, int cnt);

#endif

