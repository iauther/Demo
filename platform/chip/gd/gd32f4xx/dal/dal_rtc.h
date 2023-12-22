#ifndef __DAL_RTC_Hx__
#define __DAL_RTC_Hx__


#include "types.h"
#include "datetime.h"

int dal_rtc_init(void);
int dal_rtc_deinit(void);

int dal_rtc_get_time(datetime_t *dt);
int dal_rtc_set_time(datetime_t *dt);

int dal_rtc_get_timestamp(U32 *ts);
int dal_rtc_get_timestamp_ms(U64 *ts);
int dal_rtc_set_timestamp(U64 ts);

#endif

