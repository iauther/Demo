#include "bio/bio.h"
#include "drv/gpio.h"
#include "drv/htimer.h"
#include "drv/delay.h"
#include "led.h"
#include "myCfg.h"


typedef struct {
    U8          r;
    U8          g;
    U8          b;
}led_pins_t;

typedef struct {
    gpio_pin_t   r;
    gpio_pin_t   g;
    gpio_pin_t   b;
}led2_pins_t;

typedef struct {
    U16     light_time;
    U16     dark_time;
}led_time_t;

typedef struct {
    U8              stat;    //0:dark, 1:light
    led_cfg_t       cfg;
    led_time_t      time;
    htimer_set_t    sett;
}led_info_t;

static int led_tmr_id=0;
static led_info_t led_info;


static void led_tmr_callback(void *user_data)
{
    led_info_t *info=(led_info_t*)user_data;
    
    if(info->stat==1) {  //light
        info->time.light_time += info->sett.ms;
        if(info->time.light_time%info->cfg.light_time==0) {
            led_set_color(BLANK);
            info->stat = 0;
        }
    }
    else {
        info->time.dark_time += info->sett.ms;
        if(info->time.dark_time%info->cfg.dark_time==0) {
            led_set_color(info->cfg.blink_color);
            info->stat = 1;
        }
    }
    
    if(info->cfg.total_time>0 && info->time.light_time+info->time.dark_time >= info->cfg.total_time) {
        htimer_stop(led_tmr_id);
        led_set_color(info->cfg.stop_color);
        info->stat = 0;
        info->time.light_time = 0;
        info->time.dark_time = 0;
    }
}


int led_init(void)
{
#ifdef BOARD_PUMP_F412RET6
    U8 on,off;
    
    
    led2_pins_t pin={
                    .r=GPIO_LEDR_PIN,
                    .g=GPIO_LEDG_PIN,
                    .b=GPIO_LEDB_PIN,
    };
    
    gpio_init(&pin.r, MODE_OUTPUT);
    gpio_init(&pin.g, MODE_OUTPUT);
    gpio_init(&pin.b, MODE_OUTPUT);
    led_set_color(BLANK);
    
    led_tmr_id = htimer_new();
    //led_test();
    
#endif
    
    return 0;
}


int led_deinit(void)
{
    return htimer_free(led_tmr_id);
}



int led_set_color(U8 color)
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
#elif defined BOARD_PUMP_F412RET6
    led2_pins_t pin={
                    .r=GPIO_LEDR_PIN,
                    .g=GPIO_LEDG_PIN,
                    .b=GPIO_LEDB_PIN,
    };

    on=((LED_ON_LEVEL>0)?1:0); off=!on;
    switch(color) {
        case RED:
        gpio_set_hl(&pin.r, on);
        gpio_set_hl(&pin.g, off);
        gpio_set_hl(&pin.b, off);
        break;      
                    
        case BLUE:  
        gpio_set_hl(&pin.r, off);
        gpio_set_hl(&pin.g, off);
        gpio_set_hl(&pin.b, on);
        break;      
                    
        case GREEN: 
        gpio_set_hl(&pin.r, off);
        gpio_set_hl(&pin.g, on);
        gpio_set_hl(&pin.b, off);
        break;      
                    
        case BLANK: 
        gpio_set_hl(&pin.r, off);
        gpio_set_hl(&pin.g, off);
        gpio_set_hl(&pin.b, off);
        break;
    }
#endif
    
    return 0;
}


int led_blink_start(led_cfg_t *cfg)
{
    if(!cfg) {
        return -1;
    }
    
    led_info.stat = 1;
    led_info.cfg = *cfg;
    
    led_info.sett.ms = 1;
    led_info.sett.freq = 0;
    led_info.sett.repeat = 1;
    led_info.sett.user_data = &led_info;
    led_info.sett.callback = led_tmr_callback;
    
    led_info.time.light_time = 0;
    led_info.time.dark_time = 0;
    htimer_set(led_tmr_id, &led_info.sett);
    
    htimer_stop(led_tmr_id);
    return htimer_start(led_tmr_id);
}


int led_blink_stop(void)
{
    return htimer_stop(led_tmr_id);
}



int led_test(void)
{
    led_cfg_t cfg={RED, BLANK, 100, 100, 5000};

#if 0
    while(1) {
        led_set_color(RED);
        delay_ms(1000);
        
        led_set_color(GREEN);
        //printf("___ led on\r\n");
        delay_ms(1000);
        
        led_set_color(BLUE);
        delay_ms(1000);
        
        led_set_color(BLANK);
        //printf("___ led off\r\n");
        delay_ms(1000);
    }
#endif
    
    led_blink_start(&cfg);
    //while(1);
    
    return 0;
}

