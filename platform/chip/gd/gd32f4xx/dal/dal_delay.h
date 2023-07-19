#ifndef __DAL_DELAY_Hx__
#define __DAL_DELAY_Hx__

#include "types.h"

void dal_delay_ns(U32 ns);
void dal_delay_us(U32 us);
void dal_delay_ms(U32 ms);
void dal_delay_s(U32 s);

U64 dal_get_tick_us(void);
U32 dal_get_tick_ms(void);

#endif

