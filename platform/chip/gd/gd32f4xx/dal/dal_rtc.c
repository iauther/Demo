#include "dal_rtc.h"
#include "dal_delay.h"
#include "gd32f4xx_rtc.h"
#include "log.h"
#include "cfg.h"

#define BKP_VALUE    0x32F0
#define RTC_CLOCK_SOURCE_LXTAL

#define ALARM_TIME_INTERVAL  (2U)


rtc_timestamp_struct rtc_timestamp;
rtc_parameter_struct rtc_initpara;
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
    /* setup RTC time value */
    uint32_t tmp_hh = 0xFF, tmp_mm = 0xFF, tmp_ss = 0xFF;

    rtc_initpara.factor_asyn = prescaler_a;
    rtc_initpara.factor_syn  = prescaler_s;
    rtc_initpara.year = 0x17;
    rtc_initpara.day_of_week = RTC_SATURDAY;
    rtc_initpara.month = RTC_APR;
    rtc_initpara.date = 0x30;
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_initpara.am_pm = RTC_AM;
    
    rtc_initpara.hour   = tmp_hh;
    rtc_initpara.minute = tmp_mm;
    rtc_initpara.second = tmp_ss;

    /* RTC current time configuration */
    if(ERROR == rtc_init(&rtc_initpara)){
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
static void wakeup_config(void)
{
	rtc_wakeup_disable();
	rtc_flag_clear(RTC_FLAG_WT);                            //清空中断标志
  	exti_flag_clear(EXTI_22);
	
	exti_interrupt_flag_clear(EXTI_22);	
	nvic_irq_enable(RTC_WKUP_IRQn, 3U, 0U);
	exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);   //上升沿
	
	rtc_wakeup_clock_set(WAKEUP_CKSPRE);                    //设置RTC自动唤醒定时器时钟
	rtc_wakeup_timer_set(SAMPLE_INV_TIME-1);                //自动唤醒定时器重载值，定时一秒
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
            LOGD("power on reset occurred....\n\r");
        }else if (RESET != rcu_flag_get(RCU_FLAG_EPRST)){
            LOGD("external reset occurred....\n\r");
        }
    }

    wakeup_config();
    
    return 0;
}


int dal_rtc_set(U32 wake_time)
{
    return 0;
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




