#include <time.h>
#include "dal_rtc.h"
#include "dal_delay.h"
#include "gd32f4xx_rtc.h"
#include "log.h"
#include "cfg.h"

#define BKP_VALUE    0x32F0
#define RTC_CLOCK_SOURCE_LXTAL

#define ALARM_TIME_INTERVAL  (2U)


__IO uint32_t prescaler_a = 0, prescaler_s = 0;
static void clk_config(void)
{
#if defined (RTC_CLOCK_SOURCE_IRC32K)
      rcu_osci_on(RCU_IRC32K);
      rcu_osci_stab_wait(RCU_IRC32K);
      rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);

      prescaler_s = 0x13F;
      prescaler_a = 0x63;
#elif defined (RTC_CLOCK_SOURCE_LXTAL)
      rcu_osci_on(RCU_LXTAL);
      rcu_osci_stab_wait(RCU_LXTAL);
      rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);
      prescaler_s = 0xFF;
      prescaler_a = 0x7F;
#else
    #error RTC clock source should be defined.
#endif /* RTC_CLOCK_SOURCE_IRC32K */

    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();
}

static void rtc_setup(void)
{
    rtc_parameter_struct init;

    init.factor_asyn = prescaler_a;
    init.factor_syn  = prescaler_s;
    init.year = 0x23;
    init.day_of_week = RTC_SATURDAY;
    init.month = RTC_APR;
    init.date = 0x30;
    init.display_format = RTC_24HOUR;
    init.am_pm = RTC_AM;
    
    init.hour   = 0;
    init.minute = 0;
    init.second = 0;

    /* RTC current time configuration */
    if(ERROR == rtc_init(&init)){
        LOGE("RTC time configuration failed!\n");
    }else{
        LOGD("RTC time configuration success!\n");
        RTC_BKP0 = BKP_VALUE;
    }
}
static void alarm_setup(void)
{
    rtc_alarm_struct  rtc_alarm;
    
    rtc_alarm_disable(RTC_ALARM0);
    rtc_alarm.alarm_mask = RTC_ALARM_DATE_MASK | RTC_ALARM_HOUR_MASK | RTC_ALARM_MINUTE_MASK;
    rtc_alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    rtc_alarm.alarm_day = 0x31;
    rtc_alarm.am_pm = RTC_AM;

    /* RTC alarm value */
    rtc_alarm.alarm_hour = 0x00;
    rtc_alarm.alarm_minute = 0x00;
    rtc_alarm.alarm_second = ALARM_TIME_INTERVAL;

    /* configure RTC alarm0 */
    rtc_alarm_config(RTC_ALARM0, &rtc_alarm);

    rtc_interrupt_enable(RTC_INT_ALARM0);
    rtc_alarm_enable(RTC_ALARM0);
    
    exti_deinit();
    rtc_flag_clear(RTC_FLAG_ALRM0);
    exti_interrupt_flag_clear(EXTI_17);
    exti_interrupt_enable(EXTI_17);
}
static void wakeup_setup(void)
{
	rtc_wakeup_disable();
	rtc_flag_clear(RTC_FLAG_WT);                            //清空中断标志
  	exti_flag_clear(EXTI_22);
	
	exti_interrupt_flag_clear(EXTI_22);	
	nvic_irq_enable(RTC_WKUP_IRQn, 3U, 0U);
	exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);   //上升沿
	
	rtc_wakeup_clock_set(WAKEUP_CKSPRE);                    //设置RTC自动唤醒定时器时钟
//rtc_wakeup_timer_set(SAMPLE_INV_TIME-1);                //自动唤醒定时器重载值，定时一秒
	rtc_wakeup_enable();
	rtc_interrupt_enable(RTC_INT_WAKEUP);                   //使能中断
}




int dal_rtc_init(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
	
    clk_config();

    /* check if RTC has aready been configured */
    if ((BKP_VALUE != RTC_BKP0) || (0x00 == GET_BITS(RCU_BDCTL, 8, 9))){
        rtc_setup();
    }
    else{
        /* detect the reset source */
        if (RESET != rcu_flag_get(RCU_FLAG_PORRST)){
            LOGE("power on reset occurred....\n");
        }else if (RESET != rcu_flag_get(RCU_FLAG_EPRST)){
            LOGE("external reset occurred....\n");
        }
    }
    
    return 0;
}


static int dec2bsd(int dec)
{
    return (dec+(dec/10)*6);
}
static int bcd2dec(int bcd)
{
    return (bcd-(bcd>>4)*6);
}


int dal_rtc_set(date_time_t *dt)
{
    rtc_parameter_struct para;

    para.factor_asyn = prescaler_a;
    para.factor_syn  = prescaler_s;
    para.year = dec2bsd(dt->date.year-2000);
    para.day_of_week = dt->date.week+1;
    para.month = dt->date.mon;
    para.date = dec2bsd(dt->date.day);
    para.display_format = RTC_24HOUR;
    para.am_pm = RTC_AM;
    
    para.hour   = dt->time.hour;
    para.minute = dec2bsd(dt->time.min);
    para.second = dec2bsd(dt->time.sec);
    
    if(ERROR == rtc_init(&para)){
        LOGE("RTC time configuration failed!\n");
    }
    
    return 0;
}

static U32 get_ss(void)
{
    U32 ssc=rtc_subsecond_get();
    
    return 1000*(prescaler_s-ssc)/(prescaler_s+1);
}

int dal_rtc_get(date_time_t *dt)
{
    rtc_parameter_struct para;
    
    if(!dt) {
        return -1;
    }
    
    rtc_current_time_get(&para);
    
    dt->date.year = bcd2dec(para.year)+2000;
    dt->date.week = para.day_of_week-1;
    dt->date.mon = para.month;
    dt->date.day = bcd2dec(para.date);
    
    dt->time.hour = para.hour;
    dt->time.min = bcd2dec(para.minute);
    dt->time.sec = bcd2dec(para.second);
    dt->time.ms  = get_ss();
    
    return 0;
}


static U32 mk_time(date_time_t *dt)
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
static U32 mk_time2(date_time_t *dt)
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



U64 dal_rtc_get_timestamp(void)
{
#if 0
    static U32 s,ss;
    static U64 ms=0;
#else
    U32 s,ss;
    U64 ms=0;
#endif
    date_time_t dt;
    
    dal_rtc_get(&dt);
    s = mk_time2(&dt);
    ss = dt.time.ms;
    
    ms = ((U64)s*1000)+ss; 
    
    return ms;
}




void RTC_WKUP_IRQHandler(void)
{
	if (rtc_flag_get(RTC_FLAG_WT) != RESET) {	
		exti_interrupt_flag_clear(EXTI_22);		
		rtc_flag_clear(RTC_FLAG_WT);
	}
}
void RTC_Alarm_IRQHandler(void)
{
	if (rtc_flag_get(RTC_FLAG_ALRM0) != RESET) {
		exti_interrupt_flag_clear(EXTI_17);		
		rtc_flag_clear(RTC_FLAG_ALRM0);
	}
}




