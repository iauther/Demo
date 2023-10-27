#ifndef __RTC_Hx__
#define __RTC_Hx__

#include "types.h"
#include "dal_rtc.h"

int rtc2_init(void);
int rtc2_get_time(date_time_t *dt);
int rtc2_set_time(date_time_t *dt);
int rtc2_set_alarm(date_time_t *dt);
int rtc2_set_countdown(U32 sec);
U32 rtc2_get_timestamp_s(void);
U64 rtc2_get_timestamp_ms(void);
int rtc2_is_synced(void);
int rtc2_get_interval(date_time_t *d1, date_time_t *d2);
U8 rtc2_get_psrc(void);
U32 rtc2_get_runtime(void);
int rtc2_ts_to_dt(date_time_t *dt, U32 ts);

int rtc2_print_reg(U8 reg_start, int reg_cnt);
void rtc2_print_time(char *s, date_time_t *dt);
#endif

