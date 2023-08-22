#include "rtc.h"
#include "dal_rtc.h"
#include "date.h"
#include "sd30xx.h"
#include "log.h"


#define RETRY_TIMES   5

static handle_t hpwr=NULL;
const static date_time_t dflt_time={
    .date = {
        .year  = 2023,
        .mon   = 8,
        .day   = 11,
    },
    
    .time = {
        .hour = 8,
        .min  = 0,
        .sec  = 0,
    }
};

static void print_time(char *s, date_time_t *dt)
{
    LOGD("%s %4d.%02d.%02d-%02d-%02d:%02d:%02d\n", s, dt->date.year,dt->date.mon,dt->date.day,
                                                   dt->date.week,dt->time.hour,dt->time.min,dt->time.sec);
}

static int rtcx_first(void)
{
    int i,first=0;
    
    for(i=0; i<RETRY_TIMES; i++) {
        first = sd30xx_first_run();
        if(first>=0) {
            break;
        }
    }
    
    return first;
}
static int rtcx_quick_power_on(void)
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
static void set_power(U8 on)
{
    dal_gpio_set_hl(hpwr, !on);
}


int rtcx_init(void)
{
    int r,first;
    date_time_t dt;
    
    gpio_config();
    sd30xx_init();
    dal_rtc_init();
    
    first = rtcx_first();
    LOGD("__rtcx first: %d\n", first);
    if(first<0) {
        LOGE("__rtcx read first failed\n");
    }
    
    if(first>0) {
        dt = dflt_time;
        dt.date.week = get_week(dt.date.year, dt.date.mon, dt.date.day);
        
        r = rtcx_write_time(&dt);
        if(r) {
            LOGE("__rtcx write time failed\n");
        }
    }
    
    r = rtcx_quick_power_on();
    if(r) {
        LOGE("__rtcx quick power on failed!\n");
    }
    
    return 0;
}


int rtcx_read_time(date_time_t *dt)
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
    }
    
    if(r==0) {
        dt->date.year = tm.year+2000;
        dt->date.mon  = tm.month;
        dt->date.day  = tm.day;
        dt->date.week = tm.week;
        dt->time.hour = tm.hour;
        dt->time.min  = tm.minute;
        dt->time.sec  = tm.second;
        
        dal_rtc_set(dt);
    }
    
    return r;
}


int rtcx_write_time(date_time_t *dt)
{
    int i,r;
    sd_time_t tm;
    
    if(!dt) {
        return -1;
    }
    
    tm.year   = dt->date.year-2000;
    tm.month  = dt->date.mon;
    tm.day    = dt->date.day;
    tm.week   = dt->date.week;
    tm.hour   = dt->time.hour;
    tm.minute = dt->time.min;
    tm.second = dt->time.sec;
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = sd30xx_write_time(&tm);
        if(r==0) {
            break;
        }
    }
    dal_rtc_set(dt);
    
    return r;
}


int rtcx_set_alarm(date_time_t *dt)
{
    int i,r;
    sd_time_t tm;
    
    if(!dt) {
        return -1;
    }
    
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
            set_power(0);
            break;
        }
    }
    
    return r;
}


int rtcx_set_countdown(U32 sec)
{
    int i,r;
    sd_countdown_t cd;
    
    cd.im  = 0;             //设置为周期性中断
    cd.clk = S_1s;          //倒计时中断源选择1s
    cd.val = sec;           //倒计时初值设置为sec
    
    for(i=0; i<RETRY_TIMES; i++) {
        r = sd30xx_set_countdown(&cd);
        if(r==0) {
            r = sd30xx_clr_irq();
            if(r==0) {
                set_power(0);
                break;
            }
        }
    }
    
    return r;
}


U64 rtcx_get_timestamp(void)
{
    return dal_rtc_get_timestamp();
}



int rtcx_print(U8 reg_start, int reg_cnt)
{
    int r;
    U8 i,tmp[100];
    
    r = sd30xx_read(reg_start, tmp, reg_cnt);
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


