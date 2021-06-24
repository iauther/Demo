#include "drv/gpio.h"
#include "drv/htimer.h"
#include "drv/delay.h"
#include "buzzer.h"
#include "myCfg.h"

//不能使用PA4，没搞清楚为啥，gpio一初始化会把串口很多值改变

//蜂鸣器高电平响，低电平关


typedef struct {
    U16     ring_time;
    U16     quiet_time;
}buzzer_time_t;

typedef struct {
    U8              stat;    //0:quiet, 1:ring
    buzzer_cfg_t    cfg;
    buzzer_time_t   time;
    htimer_set_t    sett;
}buzzer_info_t;

static int buzzer_tmr_id=0;
static buzzer_info_t buzzer_info;


static void buzzer_tmr_callback(void *user_data)
{
    gpio_pin_t pin=GPIO_BUZZER_PIN;
    buzzer_info_t *info=(buzzer_info_t*)user_data;
    
    if(info->stat==1) {  //ring
        info->time.ring_time += info->sett.ms;
        if(info->time.ring_time%info->cfg.ring_time==0) {
            gpio_set_hl(&pin, 0);
            info->stat = 0;
        }
        else {
            gpio_set_hl(&pin, 1);
        }
    }
    else {
        info->time.quiet_time += info->sett.ms;
        if(info->time.quiet_time%info->cfg.quiet_time==0) {
            gpio_set_hl(&pin, 1);
            info->stat = 1;
        }
    }
    
    if(info->time.ring_time+info->time.quiet_time >= info->cfg.total_time) {
        htimer_stop(buzzer_tmr_id);
        gpio_set_hl(&pin, 0);
        info->stat = 0;
        info->time.ring_time = 0;
        info->time.quiet_time = 0;
    }
}



int buzzer_init(void)
{
    gpio_pin_t pin=GPIO_BUZZER_PIN;
    gpio_init(&pin, MODE_OUTPUT);
    gpio_set_hl(&pin, 0);
    
    buzzer_tmr_id = htimer_new();
    //buzzer_test();
    
    return 0;
}


int buzzer_deinit(void)
{
    gpio_pin_t pin=GPIO_BUZZER_PIN;
    
    return gpio_deinit(&pin);
}


int buzzer_start(buzzer_cfg_t *cfg)
{
    if(!cfg) {
        return -1;
    }
    
    buzzer_info.stat = 1;
    buzzer_info.cfg = *cfg;
    
    buzzer_info.sett.ms = 1;
    buzzer_info.sett.freq = 0;
    buzzer_info.sett.repeat = 1;
    buzzer_info.sett.user_data = &buzzer_info;
    buzzer_info.sett.callback = buzzer_tmr_callback;
    
    buzzer_info.time.ring_time = 0;
    buzzer_info.time.quiet_time = 0;
    htimer_set(buzzer_tmr_id, &buzzer_info.sett);
    
    htimer_stop(buzzer_tmr_id);
    return htimer_start(buzzer_tmr_id);
}


int buzzer_stop(void)
{
    return htimer_stop(buzzer_tmr_id);
}


int buzzer_test(void)
{
    gpio_pin_t pin=GPIO_BUZZER_PIN;
    buzzer_cfg_t cfg={100, 100, 5000};

#if 0    
    while(1) {
        gpio_set_hl(&pin, 0);
        delay_ms(500);
        gpio_set_hl(&pin, 1);
        delay_ms(500);
    }
#endif
    
    buzzer_start(&cfg);
    //while(1);
    
    return 0;
}




