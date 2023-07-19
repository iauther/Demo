#ifndef __DATE_Hx__
#define __DATE_Hx__

#include "types.h"


int get_week(U16 y, U8 m, U8 d);
int get_weekn(U16 y, U8 m, U8 d);
int get_date(char *__date__, date_s_t *date);
int get_time(char *__time__, time_s_t *time);

int get_date_string(char *__date__, void *str);
int get_time_string(char *__time__, void *str);
int date_is_valid(date_s_t *date);
int get_datetime(char *datetime, date_time_t *dt);

#endif

