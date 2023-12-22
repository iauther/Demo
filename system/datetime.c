#include <time.h>
#include <ctype.h>
#include <string.h>
#include "datetime.h"


//__date__: "Jul 27 2012"
//__TIME__: "21:06:19"

const char *sys_date=__DATE__;
const char *sys_time=__TIME__;

static U16 get_year(const char *date)
{
    U16 year=((date[7]-'0')*1000 + (date[8]-'0')*100 + (date[9]-'0')*10 + date[10]-'0');
    
    return year;
}
static U8 get_month(const char *date)
{
    U8 mon;
    
    if(date[0] == 'J' && date[1] == 'a' && date[2] == 'n')
        mon = 1;
    else if(date[0] == 'F' && date[1] == 'e' && date[2] == 'b')
        mon = 2;
    else if(date[0] == 'M' && date[1] == 'a' && date[2] == 'r')
        mon = 3;
    else if(date[0] == 'A' && date[1] == 'p' && date[2] == 'r')
        mon = 4;
    else if(date[0] == 'M' && date[1] == 'a' && date[2] == 'y')
        mon = 5;
    else if(date[0] == 'J' && date[1] == 'u' && date[2] == 'n')
        mon = 6;
    else if(date[0] == 'J' && date[1] == 'u' && date[2] == 'l')
        mon = 7;
    else if(date[0] == 'A' && date[1] == 'u' && date[2] == 'g')
        mon = 8;
    else if(date[0] == 'S' && date[1] == 'e' && date[2] == 'p')
        mon = 9;
    else if(date[0] == 'O' && date[1] == 'c' && date[2] == 't')
        mon = 10;
    else if(date[0] == 'N' && date[1] == 'o' && date[2] == 'v')
        mon = 11;
    else if(date[0] == 'D' && date[1] == 'e' && date[2] == 'c')
        mon = 12;
        
    return mon;
}
static U8 get_day(const char *date)
{
    int day,year;
    char tmp[10];
    
    sscanf((const char*)date, "%s %d %d", tmp, &day, &year);
    
    return (U8)day;
}
static U8 get_hour(const char *time)
{
    U8 hour = ((time[0] - '0') * 10 + time[1] - '0');
    
    return hour;
}
static U8 get_min(const char *time)
{
    U8 min = ((time[3] - '0') * 10 + time[4] - '0');
    
    return min;
}
static U8 get_sec(const char *time)
{
    U8 sec = ((time[6] - '0') * 10 + time[7] - '0');
    
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


int get_date(char *date_s, date_s_t *date)
{
    if(!date) {
        return -1;
    }
    
    date->year = get_year(date_s);
    date->mon  = get_month(date_s);
    date->day  = get_day(date_s);
    date->week = get_week(date->year,date->mon,date->day);
    
    return 0;
}


int get_tm(char *time_s, time_s_t *time)
{
    if(!time) {
        return -1;
    }
    
    time->hour = get_hour(time_s);
    time->min  = get_min(time_s);
    time->sec  = get_sec(time_s);
    
    return 0;
}


int get_date_str(void *str, int len)
{
    if(!str || !len) {
        return -1;
    }
    
    snprintf((char*)str, len, "%d%02d%02d", get_year(sys_date), get_month(sys_date), get_day(sys_date));
    
    return 0;
}


int get_time_str(void *str, int len)
{
    if(!str || !len) {
        return -1;
    }
    
    snprintf((char*)str, len, "%02d%02d%02d", get_hour(sys_time), get_min(sys_time), get_sec(sys_time));
    
    return 0;
}


int get_datetime_str(void *str, int len)
{
    if(!str || !len) {
        return -1;
    }
    snprintf((char*)str, len, "%d%02d%02d_%d%02d%02d", get_year(sys_date), get_month(sys_date), get_day(sys_date), get_hour(sys_time), get_min(sys_time), get_sec(sys_time));
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
int get_datetime(char *datetime, datetime_t *dt)
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


//////////////////////////////////////
static U32 get_ts(datetime_t *dt)
{
    U32 ts;
    struct tm tt;
    
    tt.tm_year = dt->date.year-1900;
    tt.tm_mon = dt->date.mon-1;
    tt.tm_mday =dt->date.day;
    tt.tm_hour = dt->time.hour;
    tt.tm_min = dt->time.min;
    tt.tm_sec = dt->time.sec;
    tt.tm_isdst = 0;
    ts = mktime(&tt)-8*3600;
    
    return ts;
}
static U32 get_s2(datetime_t *dt)
{
    U32 days,secs;
    U16 year=dt->date.year;
	U16 mon=dt->date.mon;
    U8 day=dt->date.day;
    U8 hour=dt->time.hour;
    U8 min=dt->time.min;
    U8 sec=dt->time.sec;
 
	/* 1..12 -> 11,12,1..10 */
	if (0 >= (int) (mon -= 2)) {
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}
    
    days = (year/4 - year/100 + year/400 + 367*mon/12 + day) + year*365 - 719499;
    secs = ((days*24 + hour)*60 + min)*60 + sec - 8*3600;
    
	return secs;
}


int tm_to_ts(datetime_t *dt, U64 *ts_ms)
{
    U32 ts;
    
    if(!dt || !ts_ms) {
        return -1;
    }
    
    dt->date.week = get_week(dt->date.year, dt->date.mon, dt->date.day);
    ts = get_ts(dt);
    *ts_ms = (U64)ts*1000+dt->time.ms;
    
    return 0;
}


int ts_to_tm(U64 ts_ms, datetime_t *dt)
{
    U32 days=0,rem=0;
    U16 year,month;
    U32 ts=ts_ms/1000;
    static const U16 days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if(!dt || !ts_ms) {
        return -1;
    }
    
    /* 北京时间补偿 */
    ts += 8*60*60;
 
    // 计算天数
    days = (U32)(ts / 86400);
    rem = (U32)(ts % 86400);
    
    for (year=1970 ;; year++) {
        U16 leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
        U16 ydays = leap ? 366 : 365;
        if (days < ydays) {
            break;
        }
        days -= ydays;
    }
    dt->date.year  =  year;
 
 
    for (month = 0; month < 12; month++) {
        U16 mdays = days_in_month[month];
        if (month == 1 && (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))) {
            mdays = 29;
        }
        if (days < mdays)
        {
            break;
        }
        days -= mdays;
    }
    dt->date.mon = month+1;
    dt->date.day = days + 1;
    
    dt->date.week = get_week(dt->date.year, dt->date.mon, dt->date.day);
 
    dt->time.hour = rem / 3600;
    rem %= 3600;
    dt->time.min = rem / 60;
    dt->time.sec = rem % 60;
    dt->time.ms  = ts_ms%1000;
    
    return 0;
}







