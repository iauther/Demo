#ifndef __LED_Hx__
#define __LED_Hx__

#include "types.h"


enum {
    BLANK=0,
    RED,
    BLUE,
    GREEN,
};


typedef struct {
    U8      blink_color;
    U8      stop_color;
    U16     light_time;
    U16     dark_time;
    S32     total_time;
}led_cfg_t;


int led_init(void);
int led_deinit(void);
int led_set_color(U8 color);
int led_blink_start(led_cfg_t *cfg);
int led_blink_stop(void);
int led_test(void);

#endif

