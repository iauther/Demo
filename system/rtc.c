#include "rtc.h"
#include "lock.h"
#include "paras.h"
#include "sd30xx.h"
#include "dal_gpio.h"
#include "dal_delay.h"
#include "log.h"
#include "cfg.h"


#define RETRY_TIMES   5



static U8 s_psrc=1;
static int s_first=0;
static U8 s_synced=0;
static U8 s_inited=0;
static U32 s_tstamp=0;
static U32 s_runtime=0;
static sd_countdown_t s_countdown;
static handle_t s_lock=NULL;

static U8 dec2bcd(U8 x)
{
    return (x+(x/10)*6);
}
static U8 bcd2dec(U8 x)
{
    return (x-(x>>4)*6);
}

static void print_time(char *s, datetime_t *dt)
{
    LOGD("%s %4d.%02d.%02d %02d:%02d:%02d\n", s, dt->date.year,dt->date.mon,dt->date.day,
                                                 dt->time.hour,dt->time.min,dt->time.sec);
}
static void print_stime(char *s, sd_time_t *st)
{
    LOGD("%s %4d.%02d.%02d-%02d-%02d:%02d:%02d\n", s, bcd2dec(st->year), bcd2dec(st->month), bcd2dec(st->day),
                                                   bcd2dec(st->week), bcd2dec(st->hour), bcd2dec(st->minute), bcd2dec(st->second));
}


static int get_sd_info(sd_info_t *info)
{
    int i,r;
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = sd30xx_get_info(info);
        if(r>=0) {
            break;
        }
        LOGW("___ sd30xx_get_info failed %d\n", i);
    }
    
    if(r) {
        info->first = -1;
    }
    
    return r;
}

///////////////////////////////////////
static void gpio_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    
    gpio_bit_reset(GPIOA,GPIO_PIN_1);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_25MHZ, GPIO_PIN_1);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_1);
}

static u8 get_psrc(sd_info_t *info)
{
    u8 psrc=PWRON_RTC;
    
    //如果启动时该引脚为低电平，且中断标志不为0，则为rtc中断触发开机
    if(gpio_output_bit_get(GPIOA,GPIO_PIN_1)==RESET) {
        if(info->irq==IRQ_NONE) {
            psrc = PWRON_TIMER;
        }
        else {
            psrc = PWRON_RTC;
        }
    }
    else {   //人为手动开机
        psrc = PWRON_MANUAL;
    }
    
    return 0;
}
static void rtc_ext_power(U8 on)
{
    if(on) {    //拉低上电
        gpio_bit_reset(GPIOA,GPIO_PIN_1);
    }
    else {      //拉高断电
        gpio_bit_set(GPIOA,GPIO_PIN_1);
    }
}


static int rtc_int_get(datetime_t *dt)
{
    return dal_rtc_get_time(dt);
}
static U32 rtc_int_get_ts(void)
{
    U32 ts;
    
    dal_rtc_get_timestamp(&ts);
    return ts;
}

static inline U32 to_ts(datetime_t *dt)
{
    U64 v;
    tm_to_ts(dt, &v);
    
    return v/1000;
}
static inline U64 to_ts2(datetime_t *dt)
{
    U64 v;
    tm_to_ts(dt, &v);
    
    return v;
}
static inline datetime_t to_tm(U64 ts)
{
    datetime_t dt;
    ts_to_tm(ts, &dt);
    
    return dt;
}

static int rtc_int_set(datetime_t *dt)
{
    U64 v;
    U32 ts1,ts2,ts;
    
    //datetime_t dt1,dt2;;
    //dal_rtc_set_time(&dt);
    //dal_rtc_get_time(&dt2);
    
    if(s_tstamp) {
        dal_rtc_get_timestamp(&ts1);
        dal_rtc_set_time(dt);
        dal_rtc_get_timestamp(&ts2);
    }
    else {
        dal_rtc_set_time(dt);
        
        ts1 = 0;
        dal_rtc_get_timestamp(&ts2);
    }
    
    //ts = to_ts(dt);
    //LOGD("__1__ ts1: %lu, ts2: %lu, s_ts: %lu, s_runtime: %lu\n", ts1, ts2, s_tstamp, s_runtime);
    s_runtime += ts1?(ts1-s_tstamp):0;
    s_tstamp = ts2;
    //LOGD("__2__ ts1: %lu, ts2: %lu, s_ts: %lu, s_runtime: %lu\n", ts1, ts2, s_tstamp, s_runtime);
    //LOGD("__3__ write_ts: %lu, read_ts: %lu\n", ts, ts2);
    
    return 0;
}

static int rtc_ext_get(datetime_t *dt)
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
static int rtc_ext_set(datetime_t *dt)
{
    int i,r;
    U8  week;
    
    sd_time_t tm1={0},tm2={0};
    
    if(!dt) {
        return -1;
    }
    
    week = get_week(dt->date.year, dt->date.mon, dt->date.day);
    tm1.year   = dec2bcd(dt->date.year-2000);
    tm1.month  = dec2bcd(dt->date.mon);
    tm1.day    = dec2bcd(dt->date.day);
    tm1.week   = dec2bcd(week);
    tm1.hour   = dec2bcd(dt->time.hour);
    tm1.minute = dec2bcd(dt->time.min);
    tm1.second = dec2bcd(dt->time.sec);
    //print_stime("rtc_ext_set:", &tm1);
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = sd30xx_write_time(&tm1);
        if(r==0) {
            r = sd30xx_read_time(&tm2);
            if(r==0) {
                if(memcmp(&tm1, &tm2, sizeof(sd_time_t))==0) {
                    break;
                }
                else {
                    print_stime("rtc_ext_set:", &tm1);
                    print_stime("rtc_ext_get:", &tm2);
                }
            }
            LOGW("___ sd30xx_read_time failed %d\n", i);
        }
        LOGW("___ rtc_ext_set failed %d\n", i);
    }
    
    if(r) {
        LOGE("___ rtcx_set_time failed\n");
        return -1;
    }
    
    s_synced = 1;
    
    return r;
}
static int rtc_power_on(U8 on)
{
    int i,r=0;
    sd_countdown_t cd;
    
    if(on) {
        LOGD("___ rtc power on ...\n");
        rtc_ext_power(1);
        
#ifdef COUNTDOWN_POWER
        cd.im  = 0;
        cd.clk = S_4096Hz;
        cd.val = 1;
        r = sd30xx_set_countdown(&cd);      //第一次，尽快使RTC接通电源
        if(r) {
            LOGE("___ rtc power on failed 111\n");
            return -1;
        }
#endif
    }
    else {
        LOGD("___ rtc power off ...\n");
    
        r = sd30xx_set_countdown(&s_countdown);
        if(r) {
            LOGE("___ rtc power off failed 111\n");
            return -1;
        }
        log_save();
        
        r = sd30xx_clr_irq();
        if(r) {
            LOGE("___ rtc power off failed 222\n");
            return -1;
        }
        rtc_ext_power(0);
    }
    
    return 0;
}
////////////////////////////////////////////
int rtc2_init(void)
{
    int i,r;
    datetime_t dt;
    sd_info_t  info;
    
    gpio_init();
    dal_rtc_init();
    sd30xx_init();
    
    get_sd_info(&info);
    s_psrc = get_psrc(&info);
    s_first = info.first;
    
    LOGD("__rtc.first: %d, rtc.psrc: %d\n", s_first, s_psrc);
    if(s_first<0) {
        LOGE("__rtc2 read first failed\n");
    }
    
    if(s_first>0) {
        dt = DFLT_TIME;
        dt.date.week = get_week(dt.date.year, dt.date.mon, dt.date.day);
        
        rtc_int_set(&dt);
    }
    else {
        r = rtc_ext_get(&dt);
        if(r==0) {
            rtc_int_set(&dt);
        }
        else {
            LOGE("___ rtc_ext_get failed\n");
        }
    }
    
    r = rtc_int_get(&dt);
    if(r) {
        LOGE("___ rtc2_init, rtc_int_get failed\n");
        return -1;
    }
    print_time("rtc2_init,", &dt);
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = rtc_power_on(1);
        if(r==0) {
            break;
        }
        LOGW("___ rtc_power_on failed %d\n", i);
    }
    
    s_lock = lock_init();
    s_inited = 1;
    
    return 0;
}


int rtc2_deinit(void)
{
    dal_rtc_deinit();
    sd30xx_deinit();
    
    return 0;
}


int rtc2_get_time(datetime_t *dt)
{
    int r;
    
    lock_on(s_lock);
    r = rtc_int_get(dt);
    lock_off(s_lock);
    
    return r;
}


int rtc2_set_time(datetime_t *dt)
{
    int r;
    
    lock_on(s_lock);
    r = rtc_int_set(dt);
    lock_off(s_lock);
    
    return r;
}


int rtc2_set_alarm(datetime_t *dt)
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
    tm.week   = get_week(dt->date.year, dt->date.mon, dt->date.day);
    tm.hour   = dt->time.hour;
    tm.minute = dt->time.min;
    tm.second = dt->time.sec;
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = sd30xx_set_alarm(1, &tm);
        if(r==0) {
            rtc_power_on(1);
            break;
        }
    }
    lock_off(s_lock);
    
    return r;
}


int rtc2_set_countdown(U32 sec)
{
    int i,r;
    datetime_t dt={0};
    
    LOGD("____ rtc2_set_countdown, %d\n", sec);
    if(sec>3600*24) {
        LOGW("___ rtc2_set_countdown, sec is too large, %d\n", sec);
        return -1;
    }
    
    s_countdown.im  = 0;             //设置为周期性中断
    s_countdown.clk = S_1s;          //倒计时中断源选择1s
    s_countdown.val = sec;           //倒计时初值设置为sec
    
    lock_on(s_lock);
    
    rtc_int_get(&dt);
    r = rtc_ext_set(&dt);
    
    if(r==0) {  //关机前打印一下rtc时间
       print_time("++++", &dt);
    }
    else {
        LOGE("___ rtc2_set_countdown, failed 000\n");
    }
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = rtc_power_on(0);
        if(r==0) {
            break;
        }
        LOGW("___ rtc_power_off failed %d\n", i);
        rtc_power_on(1);
    }
    
    lock_off(s_lock);
    
    return r;
}





U32 rtc2_get_timestamp_s(void)
{
    U32 v;
    
    lock_on(s_lock);
    dal_rtc_get_timestamp(&v);
    lock_off(s_lock);
    
    return v;
}


U64 rtc2_get_timestamp_ms(void)
{
    U64 v;
    
    lock_on(s_lock);
    dal_rtc_get_timestamp_ms(&v);
    lock_off(s_lock);
    
    return v;
}


//time interval
int rtc2_get_interval(datetime_t *d1, datetime_t *d2)
{
    U32 s1,s2;
    if(!d1 || !d2) {
        return -1;
    }
    s1 = to_ts(d1); s2 = to_ts(d2);
    
    return (s1>s2)?(s1-s2):(s2-s1);
}


//compare with DFLT_TIME, if byond one day millseconds
//do think the time not synced
int rtc2_is_synced(void)
{
    int r=1;
    datetime_t dt1,dt2=DFLT_TIME;
    U32 inv,day_s=3600*24;
    
    if(s_synced) {
        return 1;
    }
    
    lock_on(s_lock);
    if(s_first>0) {
        dal_rtc_get_time(&dt1);
        inv = to_ts(&dt1) - to_ts(&dt2);
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


int rtc2_get_runtime(U32 *runtime)
{
    U32 ts,t1,t2;
    
    ts = rtc_int_get_ts();
    t1 = ts-s_tstamp;
    t2 = t1+s_runtime;
    
    //LOGD("____s_ts: %lu, ts: %lu, t1: %lu, s_runtime: %lu, t2: %lu\n", ts, s_tstamp, t1, t2);
    if(runtime) {
        *runtime = t2;
    }
    
    return 0;
}



void rtc2_print_time(char *s, datetime_t *dt)
{
    print_time(s, dt);
}


int rtc2_dump(U8 reg, U8 *data, U8 cnt)
{
    int r,i;
    
    if(!data || !cnt) {
        LOGE("___ wrong para\n");
        return -1;
    }
    
    memset(data, 0, cnt);
    sd30xx_init();
    r = sd30xx_read(reg, data, cnt);
    if(r) {
        return r;
    }
    
    return 0;
}


void rtc2_test(void)
{
    sd_time_t tm;
    
    sd30xx_read_time(&tm);
    print_stime("sss", &tm);
}



