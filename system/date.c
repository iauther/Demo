#include "date.h"


//__date__: "Jul 27 2012"
//__TIME__: "21:06:19"


static U16 get_year(char *__date__)
{
    U16 year=((__date__[7]-'0')*1000 + (__date__[8]-'0')*100 + (__date__[9]-'0')*10 + __date__[10]-'0');
    
    return year;
}
static U8 get_month(char *__date__)
{
    U8 mon;
    
    if(__date__[0] == 'J' && __date__[1] == 'a' && __date__[2] == 'n')
        mon = 1;
    else if(__date__[0] == 'F' && __date__[1] == 'e' && __date__[2] == 'b')
        mon = 2;
    else if(__date__[0] == 'M' && __date__[1] == 'a' && __date__[2] == 'r')
        mon = 3;
    else if(__date__[0] == 'A' && __date__[1] == 'p' && __date__[2] == 'r')
        mon = 4;
    else if(__date__[0] == 'M' && __date__[1] == 'a' && __date__[2] == 'y')
        mon = 5;
    else if(__date__[0] == 'J' && __date__[1] == 'u' && __date__[2] == 'n')
        mon = 6;
    else if(__date__[0] == 'J' && __date__[1] == 'u' && __date__[2] == 'l')
        mon = 7;
    else if(__date__[0] == 'A' && __date__[1] == 'u' && __date__[2] == 'g')
        mon = 8;
    else if(__date__[0] == 'S' && __date__[1] == 'e' && __date__[2] == 'p')
        mon = 9;
    else if(__date__[0] == 'O' && __date__[1] == 'c' && __date__[2] == 't')
        mon = 10;
    else if(__date__[0] == 'N' && __date__[1] == 'o' && __date__[2] == 'v')
        mon = 11;
    else if(__date__[0] == 'D' && __date__[1] == 'e' && __date__[2] == 'c')
        mon = 12;
        
    return mon;
}
static U8 get_day(char *__date__)
{
    U8 day = ((__date__[4] - '0') * 10 + __date__[5] - '0');
    
    return day;
}
static U8 get_hour(char *__time__)
{
    U8 hour = ((__time__[0] - '0') * 10 + __time__[1] - '0');
    
    return hour;
}
static U8 get_min(char *__time__)
{
    U8 min = ((__time__[3] - '0') * 10 + __time__[4] - '0');
    
    return min;
}
static U8 get_sec(char *__time__)
{
    U8 sec = ((__time__[6] - '0') * 10 + __time__[7] - '0');
    
    return sec;
}
/////////////////////////////////////////////////////////////

int get_date(char *__date__, date_t *date)
{
    if(!date) {
        return -1;
    }
    
    date->year = get_year(__date__);
    date->mon  = get_month(__date__);
    date->day  = get_day(__date__);
    
    return 0;
}


int get_time(char *__time__, time_t *time)
{
    if(!time) {
        return -1;
    }
    
    time->hour = get_hour(__time__);
    time->min  = get_min(__time__);
    time->sec  = get_sec(__time__);
    
    return 0;
}


int get_date_string(char *__date__, void *str)
{
    if(!str) {
        return -1;
    }
    
    sprintf(str, "%d%d%d", get_year(__date__), get_month(__date__), get_day(__date__));
    
    return 0;
}


int get_time_string(char *__time__, void *str)
{
    if(!str) {
        return -1;
    }
    
    sprintf(str, "%d%d%d", get_hour(__time__), get_min(__time__), get_sec(__time__));
    
    return 0;
}


int date_is_valid(date_t *date)
{
    if(!date) {
        return -1;
    }
    
    if((date->year>=2100 || date->year<=2000) || (date->mon>12 || date->mon==0) || (date->day>12 || date->day==0)) {
        return 0;
    }
    
    return 1;
}




