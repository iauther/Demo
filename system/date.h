#ifndef __DATE_Hx__
#define __DATE_Hx__

#include "types.h"


int get_week(U16 y, U8 m, U8 d);
int get_weekn(U16 y, U8 m, U8 d);
int get_date(char *date_s, date_s_t *date);
int get_time(char *time_s, time_s_t *time);

int get_date_string(char *date, void *str);
int get_time_string(char *time, void *str);
int date_is_valid(date_s_t *date);
int get_datetime(char *datetime, date_time_t *dt);
int get_timestamp(date_time_t *dt, U32 *tt);

#endif

