#ifndef __RTC_Hx__
#define __RTC_Hx__

#include "types.h"



int rtcx_init(void);
int rtcx_read_time(date_time_t *dt);
int rtcx_write_time(date_time_t *dt);
int rtcx_set_alarm(date_time_t *dt);
int rtcx_set_countdown(U32 sec);

int rtcx_print(U8 reg_start, int reg_cnt);
#endif

