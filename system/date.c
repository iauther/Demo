#include "date.h"
#include <ctype.h>
#include <string.h>

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

//https://verytoolz.com/blog/3e1f461a7d/
static U8 DOW(U16 y, U8 m, U8 d)
{
    // array with leading number of days values
    U8 t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
         
    // if month is less than 3 reduce year by 1
    if (y < 3) y -= 1;

    return ((y + y/4 - y/100 + y/400 + t[m-1] + d) % 7);
}
/////////////////////////////////////////////////////////////
int get_week(U16 y, U8 m, U8 d)
{
    return DOW(y,m,d);
}

//http://www.proesite.com/timex/wkcalc.htm
int get_weekn(U16 y, U8 m, U8 d)
{
    int dow     = DOW( y, m, d );
    int dow0101 = DOW( y, 1, 1 );

    if ((m==1)  &&  ((3<dow0101) && (dow0101< (7-(d-1)))) ) {
        // days before week 1 of the current year have the same week number as
        // the last day of the last week of the previous year

        dow     = dow0101 - 1;
        dow0101 = DOW(y-1, 1, 1);
        m       = 12;
        d       = 31;
    }
    else {
        // days after the last week of the current year have the same week number as
        // the first day of the next year, (i.e. 1)
        U8 x=DOW(y+1, 1, 1);
        if ((m==12) && ((30-(d-1)<x) && (x<4))) {
            return 1;
        }
    }

    return (DOW( y, 1, 1 ) < 4 ) + 4 * (m-1) + ( 2 * (m-1) + (d-1) + dow0101 - dow + 6) * 36 / 256;
}


int get_date(char *__date__, date_s_t *date)
{
    if(!date) {
        return -1;
    }
    
    date->year = get_year(__date__);
    date->mon  = get_month(__date__);
    date->day  = get_day(__date__);
    date->week = get_week(date->year,date->mon,date->day);
    
    return 0;
}


int get_time(char *__time__, time_s_t *time)
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
    
    sprintf(str, "%d%02d%02d", get_year(__date__), get_month(__date__), get_day(__date__));
    
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


int date_is_valid(date_s_t *date)
{
    if(!date) {
        return -1;
    }
    
    if((date->year>=2100 || date->year<=2000) || (date->mon>12 || date->mon==0) || (date->day>12 || date->day==0)) {
        return 0;
    }
    
    return 1;
}

//YYYY/MM/DD,hh:mm:ss+zz
int get_datetime(char *datetime, date_time_t *dt)
{
    char *p;
    char tmp[100];
    
    if(!datetime || !dt) {
        return -1;
    }
    
    strcpy(tmp, datetime);
    
    p = strchr(tmp, '"');
    if(!p) {
        return -1;
    }
    p++;
    
    p[4] = p[7] = p[10] = p[13] = p[16] = p[19] = 0;
    
    dt->date.year = atoi(p);
    dt->date.mon  = atoi(p+5);
    dt->date.day  = atoi(p+8);
    dt->date.week = get_week(dt->date.year, dt->date.mon, dt->date.day);
    
    dt->time.hour = atoi(p+11)+8;
    dt->time.min  = atoi(p+14);
    dt->time.sec  = atoi(p+17);

    
    return 0;
}

