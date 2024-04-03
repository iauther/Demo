#ifndef __RTC_Hx__
#define __RTC_Hx__

#include "types.h"


int rtc2_init(void);
int rtc2_deinit(void);

int rtc2_get_time(datetime_t *dt);
int rtc2_set_time(datetime_t *dt);
int rtc2_set_alarm(datetime_t *dt);
int rtc2_set_countdown(U32 sec);
U32 rtc2_get_timestamp_s(void);
U64 rtc2_get_timestamp_ms(void);
int rtc2_is_synced(void);
int rtc2_get_interval(datetime_t *d1, datetime_t *d2);
U8 rtc2_get_psrc(void);
U32 rtc2_get_runtime(void);
int rtc2_get_temp(F32 *temp);

void rtc2_print_time(char *s, datetime_t *dt);
int rtc2_dump(U8 reg, U8 *data, U8 cnt);
void rtc2_test(void);
#endif

