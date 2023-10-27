#include <time.h>
#include "rtc.h"
#include "lock.h"
#include "date.h"
#include "paras.h"
#include "sd30xx.h"
#include "dal_gpio.h"
#include "dal_delay.h"
#include "log.h"


#define RETRY_TIMES   5

static U8 s_psrc=1;
static int s_first=0;
static U8 s_synced=0;
static U8 s_inited=0;
static U32 s_tstamp=0;
static U32 s_runtime=0;
static handle_t hpwr=NULL;
static handle_t s_lock=NULL;

static U8 dec2bcd(U8 x)
{
    return (x+(x/10)*6);
}
static U8 bcd2dec(U8 x)
{
    return (x-(x>>4)*6);
}

static void print_time(char *s, date_time_t *dt)
{
    LOGD("%s %4d.%02d.%02d %02d:%02d:%02d\n", s, dt->date.year,dt->date.mon,dt->date.day,
                                                 dt->time.hour,dt->time.min,dt->time.sec);
}
static void print_stime(char *s, sd_time_t *st)
{
    LOGD("%s %4d.%02d.%02d-%02d-%02d:%02d:%02d\n", s, bcd2dec(st->year), bcd2dec(st->month), bcd2dec(st->day),
                                                   bcd2dec(st->week), bcd2dec(st->hour), bcd2dec(st->minute), bcd2dec(st->second));
}


static int is_first(void)
{
    int i,first=0;
    
    for(i=0; i<RETRY_TIMES; i++) {
        first = sd30xx_first();
        if(first>=0) {
            break;
        }
    }
    
    return first;
}
static int quick_pwron(void)
{
    int i,r;
    sd_countdown_t cd;
    
    cd.im  = 0;
    cd.clk = S_4096Hz;
    cd.val = 1;
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = sd30xx_set_countdown(&cd);      //第一次，尽快使RTC接通电源
        if(r==0) {
            break;
        }
    }
    
    return r;
}


///////////////////////////////////////
static void gpio_config(void)
{
    dal_gpio_cfg_t gc;
    
    gc.gpio.grp = GPIOA;
    gc.gpio.pin = GPIO_PIN_1;
    gc.mode = MODE_OUT_OD;
    gc.pull = PULL_NONE;
    gc.hl = 0;
    hpwr = dal_gpio_init(&gc); //配置
}
static int is_rtc_pwron(void)
{    
    //如果启动时该引脚为低电平，则为rtc中断触发开机，否则为人为手动开机
    if(dal_gpio_get_hl(hpwr)==0) {
        return 1;
    }
    
    return 0;
}
static void rtc_ext_power(U8 on)
{
    dal_gpio_set_hl(hpwr, !on);
}
static U32 get_s2(date_time_t *dt)
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
static U32 get_ts(date_time_t *dt)
{
    struct tm tt;
    
    tt.tm_year = dt->date.year-1900;
    tt.tm_mon = dt->date.mon-1;
    tt.tm_mday =dt->date.day;
    tt.tm_hour = dt->time.hour;
    tt.tm_min = dt->time.min;
    tt.tm_sec = dt->time.sec;
    tt.tm_isdst = 0;
    
    return mktime(&tt)-8*3600;
}
static U64 get_ts_ms(date_time_t *dt)
{
    U32 s,ss;
    U64 ms=0;
    
    s = get_ts(dt);
    ss = dt->time.ms;
    
    ms = ((U64)s*1000)+ss; 
    
    return ms;
}

static int rtc_int_get(date_time_t *dt)
{
    return dal_rtc_get(dt);
}
static int rtc_int_set(date_time_t *dt)
{
    U32 ts1,ts2,ts;
    date_time_t tmp,dt2;
    
    ts = get_ts(dt);
    if(s_tstamp) {
        dal_rtc_get(&tmp);
        ts1 = get_ts(&tmp);
        
        dal_rtc_set(dt);
        dal_rtc_get(&dt2);
        
        ts2 = get_ts(&dt2);
    }
    else {
        dal_rtc_set(dt);
        dal_rtc_get(&tmp);
        
        ts1 = 0;
        ts2 = get_ts(&tmp);
    }
    
    //LOGD("__1__ ts1: %lu, ts2: %lu, s_ts: %lu, s_runtime: %lu\n", ts1, ts2, s_tstamp, s_runtime);
    s_runtime += (ts1?(ts1-s_tstamp):0);
    s_tstamp = ts2;
    //LOGD("__2__ ts1: %lu, ts2: %lu, s_ts: %lu, s_runtime: %lu\n", ts1, ts2, s_tstamp, s_runtime);
    
    //LOGD("__3__ write_ts: %lu, read_ts: %lu\n", ts, ts2);
    
    return 0;
}

static int rtc_ext_get(date_time_t *dt)
{
    int i,r;
    sd_time_t tm;
    
    if(!dt) {
        return -1;
    }
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = sd30xx_read_time(&tm);
        if(r==0) {
            break;
        }
        
        LOGW("____ rtc_ext_get failed times %d\n", i+1);
    }
    
    if(r) {
        LOGE("___ rtc_ext_get failed\n");
        return -1;
    }
    
    dt->date.year = bcd2dec(tm.year)+2000;
    dt->date.mon  = bcd2dec(tm.month);
    dt->date.day  = bcd2dec(tm.day);
    dt->date.week = bcd2dec(tm.week);
    dt->time.hour = bcd2dec(tm.hour);
    dt->time.min  = bcd2dec(tm.minute);
    dt->time.sec  = bcd2dec(tm.second);
    
    return 0;
}
static int rtc_ext_set(date_time_t *dt)
{
    int i,r;
    sd_time_t tm1,tm2;
    date_time_t tmp;
    
    if(!dt) {
        return -1;
    }
    
    tm1.year   = dec2bcd(dt->date.year-2000);
    tm1.month  = dec2bcd(dt->date.mon);
    tm1.day    = dec2bcd(dt->date.day);
    tm1.week   = dec2bcd(dt->date.week);
    tm1.hour   = dec2bcd(dt->time.hour);
    tm1.minute = dec2bcd(dt->time.min);
    tm1.second = dec2bcd(dt->time.sec);
    //print_stime("rtc_ext_set:", &tm1);
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = sd30xx_write_time(&tm1);
        if(r==0) {
            sd30xx_read_time(&tm2);
            print_stime("rtc_ext_get:", &tm2);
            break;
        }
        LOGW("___ rtc_ext_set failed times: %d\n", i+1);
    }
    
    if(r) {
        LOGE("___ rtcx_set_time failed\n");
        return -1;
    }
    
    rtc_int_set(dt);
    s_synced = 1;
    
    return r;
}
////////////////////////////////////////////



int rtc2_init(void)
{
    int r;
    date_time_t dt1,dt2;
    
    gpio_config();
    dal_rtc_init();
    sd30xx_init();
    
    s_psrc = is_rtc_pwron();      //0:manual poweron   1:rtc poweron
    
    s_first = is_first();
    LOGD("__rtc.first: %d, rtc.psrc: %d\n", s_first, s_psrc);
    if(s_first<0) {
        LOGE("__rtc2 read first failed\n");
    }
    
    if(s_first>0) {
        dt1 = DFLT_TIME;
        dt1.date.week = get_week(dt1.date.year, dt1.date.mon, dt1.date.day);
        
        r = rtc_ext_set(&dt1);
        if(r) {
            LOGE("__rtc2_init, rtc_ext_set failed\n");
        }
    }
    else {
        r = rtc_ext_get(&dt1);
        if(r==0) {
            print_time("rtcx_get:", &dt1);
            rtc_int_set(&dt1);
        }
        else {
            LOGE("___ rtc_ext_get failed\n");
        }
    }
    
    r = quick_pwron();
    if(r) {
        LOGE("__quick pwron failed!\n");
    }
    s_lock = lock_init();
    
    s_inited = 1;
    
    return 0;
}


int rtc2_get_time(date_time_t *dt)
{
    int r;
    
    lock_on(s_lock);
    r = rtc_int_get(dt);
    lock_off(s_lock);
    
    return r;
}


int rtc2_set_time(date_time_t *dt)
{
    int r;
    
    lock_on(s_lock);
    r = rtc_ext_set(dt);
    lock_off(s_lock);
    
    return r;
}

int rtc2_set_alarm(date_time_t *dt)
{
    int i,r;
    sd_time_t tm;
    
    if(!dt) {
        return -1;
    }
    
    lock_on(s_lock);
    tm.year   = dt->date.year;
    tm.month  = dt->date.mon;
    tm.day    = dt->date.day;
    tm.week   = dt->date.week;
    tm.hour   = dt->time.hour;
    tm.minute = dt->time.min;
    tm.second = dt->time.sec;
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = sd30xx_set_alarm(1, &tm);
        if(r==0) {
            rtc_ext_power(0);
            break;
        }
    }
    lock_off(s_lock);
    
    return r;
}


int rtc2_set_countdown(U32 sec)
{
    int i,r;
    date_time_t dt={0};
    sd_countdown_t cd;
    
    cd.im  = 0;             //设置为周期性中断
    cd.clk = S_1s;          //倒计时中断源选择1s
    cd.val = sec;           //倒计时初值设置为sec
    
    lock_on(s_lock);
    
    rtc_ext_get(&dt);
    print_time("++++", &dt);
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = sd30xx_set_countdown(&cd);
        if(r==0) {
            rtc_ext_power(0);
            r = sd30xx_clr_irq();
            if(r==0) {
                break;
            }
        }
    }
    lock_off(s_lock);
    
    return r;
}


U32 rtc2_get_timestamp_s(void)
{
    U32 v;
    date_time_t dt;
    
    lock_on(s_lock);
    rtc_int_get(&dt);
    v = get_ts(&dt);
    lock_off(s_lock);
    
    return v;
}


U64 rtc2_get_timestamp_ms(void)
{
    U64 v;
    date_time_t dt;
    
    lock_on(s_lock);
    rtc_int_get(&dt);
    v = get_ts_ms(&dt);
    lock_off(s_lock);
    
    return v;
}


//time interval
int rtc2_get_interval(date_time_t *d1, date_time_t *d2)
{
    U32 s1,s2;
    if(!d1 || !d2) {
        return -1;
    }
    s1 = get_ts(d1); s2 = get_ts(d2);
    
    return (s1>s2)?(s1-s2):(s2-s1);
}


//compare with DFLT_TIME, if byond one day millseconds
//do think the time not synced
int rtc2_is_synced(void)
{
    int r=1;
    date_time_t dt1,dt2=DFLT_TIME;
    U32 inv,day_s=3600*24;
    
    if(s_synced) {
        return 1;
    }
    
    lock_on(s_lock);
    if(s_first>0) {
        dal_rtc_get(&dt1);
        inv = get_ts(&dt1) - get_ts(&dt2);
        if(inv<=day_s) {
            r = 0;
        }
    }
    lock_off(s_lock);
    
    return r;
}


U8 rtc2_get_psrc(void)
{
    return s_psrc;
}


U32 rtc2_get_runtime(void)
{
    U32 ts,t1,t2;
    date_time_t dt1,dt2;
    
    lock_on(s_lock);
    rtc_int_get(&dt1);
    rtc_ext_get(&dt2);
    lock_off(s_lock);
    
    //print_time("int_time:", &dt1);
    //print_time("ext_time:", &dt2);
    
    ts = get_ts(&dt1);
    t1 = ts-s_tstamp;
    t2 = ts-s_tstamp+s_runtime;
    //LOGD("____ts: %lu, s_ts: %lu, t1: %lu, s_runtime: %lu, t2: %lu\n", ts, s_tstamp, t1, t2);
    
    return t2;
}


int rtc2_print_reg(U8 reg_start, int reg_cnt)
{
    int r;
    U8 i,tmp[100];
    
    lock_on(s_lock);
    r = sd30xx_read(reg_start, tmp, reg_cnt);
    lock_off(s_lock);
    
#if 1
    if(r==0) {
        LOGD("\n");
        for(i=0; i<reg_cnt; i++) {
            LOGD("reg[0x%02x]: 0x%02x\n", reg_start+i, tmp[i]);
        }
    }
#endif
    
    return 0;
}


void rtc2_print_time(char *s, date_time_t *dt)
{
    print_time(s, dt);
}


int rtc2_ts_to_dt(date_time_t *dt, U32 ts)
{
    if(!dt) {
        return -1;
    }
    
    U32 days=0,rem=0;
    U16 year,month;
    static const U16 days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
 
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
 
    dt->time.hour = rem / 3600;
    rem %= 3600;
    dt->time.min = rem / 60;
    dt->time.sec = rem % 60;
    
    return 0;
}



