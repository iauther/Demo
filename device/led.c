#include "led.h"
#include "bio/bio.h"
#include "drv/gpio.h"
#include "drv/timer.h"
#include "drv/delay.h"


typedef struct {
    U8   r;
    U8   g;
    U8   b;
}led_pins_t;

#define LED_ON_LEVEL       1


typedef struct {
    S32     cur_time;
    S32     total_time;
}led_time_t;

typedef struct {
    int            cnt;
    led_time_t     time;
    timer_set_t    set;
    U8             color;
    handle_t       handle;
}led_info_t;

static int led_cnt=0;
static led_info_t led_info;


static void led_tmr_callback(void *user_data)
{
    led_info_t *info=(led_info_t*)user_data;
    
    if(info->cnt%2==0) {
        led_set_color(info->color);
    }
    else {
        led_set_color(BLANK);
    }
    
    info->cnt++;
    info->time.cur_time += info->set.ms;
    if(info->time.cur_time>=info->time.total_time) {
        timer_sw_stop(&info->handle);
        info->cnt = 0;
    }
}


void led_set_color(U8 color)
{
    U8 on,off;

#ifdef BOARD_V00_02
    led_pins_t p1={AD8233_GPIO_EXT_PIN_LED_R,AD8233_GPIO_EXT_PIN_LED_G,AD8233_GPIO_EXT_PIN_LED_B};
    led_pins_t p2={ADS129X_GPIO_EXT_PIN_LED_R,ADS129X_GPIO_EXT_PIN_LED_G,ADS129X_GPIO_EXT_PIN_LED_B};;
    led_pins_t pin=((bioDevice==AD8233_FREQ)?p1:p2);
    
    on=((LED_ON_LEVEL>0)?1:0); off=!on;
    switch(color) {
        case RED:
        gpio_ext_set_hl(pin.r, on);
        gpio_ext_set_hl(pin.g, off);
        gpio_ext_set_hl(pin.b, off);
        break;
        
        case BLUE:
        gpio_ext_set_hl(pin.r, off);
        gpio_ext_set_hl(pin.g, off);
        gpio_ext_set_hl(pin.b, on);
        break;
        
        case GREEN:
        gpio_ext_set_hl(pin.r, off);
        gpio_ext_set_hl(pin.g, on);
        gpio_ext_set_hl(pin.b, off);
        break;
        
        case BLANK:
        gpio_ext_set_hl(pin.r, off);
        gpio_ext_set_hl(pin.g, off);
        gpio_ext_set_hl(pin.b, off);
        break;
    }
#endif
}


void led_blink_start(U8 color, U16 gap_time, S32 total_time)
{   
    led_info.cnt = 0;
    led_info.color = color;
    
    led_info.set.ms = gap_time;
    led_info.set.freq = 0;
    led_info.set.repeat = 1;
    led_info.set.user_data = &led_info;
    led_info.set.callback = led_tmr_callback;
    
    led_info.time.cur_time = 0;
    led_info.time.total_time = total_time;
    led_info.handle = timer_sw_start(&led_info.set);
}


void led_blink_stop(void)
{
    timer_sw_stop(&led_info.handle);
}



void led_test(void)
{
    while(1) {
        //led_set_color(RED);
        //delay_ms(1000);
        
        //led_set_color(BLUE);
        //delay_ms(1000);
        
        led_set_color(GREEN);
        printf("___ led on\r\n");
        delay_s(2);
        
        led_set_color(BLANK);
        printf("___ led off\r\n");
        delay_s(2);
    }
}

