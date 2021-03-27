#ifndef __LED_Hx__
#define __LED_Hx__

#include "types.h"


enum {
    BLANK=0,
    RED,
    BLUE,
    GREEN,
};


void led_set_color(U8 color);

void led_blink_start(U8 color, U16 gap_time, S32 total_time);

void led_blink_stop(void);

void led_test(void);


















#endif

