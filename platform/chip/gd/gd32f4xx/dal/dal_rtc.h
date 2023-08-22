#ifndef __DAL_RTC_Hx__
#define __DAL_RTC_Hx__


#include "types.h"

int dal_rtc_init(void);
int dal_rtc_set(date_time_t *dt);
int dal_rtc_get(date_time_t *dt);
int dal_rtc_set(date_time_t *dt);
U64 dal_rtc_get_timestamp(void);

#endif

