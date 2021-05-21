#ifndef __DATE_Hx__
#define __DATE_Hx__

#include "types.h"

typedef struct {
    U16 year;
    U8  mon;
    U8  day;
}date_t;

typedef struct {
    U8  hour;
    U8  min;
    U8  sec;
    
}time2_t;


int get_date(char *__date__, date_t *date);
int get_time(char *__time__, time2_t *time);

int get_date_string(char *__date__, void *str);
int get_time_string(char *__time__, void *str);
int date_is_valid(date_t *date);

#endif

