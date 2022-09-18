#ifndef __NOTICE_Hx__
#define __NOTICE_Hx__

#include "buzzer.h"


enum {
    LEV_WARN=0,
    LEV_ERROR,
    LEV_UPGRADE,
    
    LEV_MAX
};

enum {
    DEV_LED=(1<<0),
    DEV_BUZZER=(1<<1),
};


typedef struct {
    led_cfg_t       led;
    buzzer_cfg_t    buzzer;
}notice_t;


int notice_start(U8 dev, U8 level);
int notice_stop(U8 dev);


#endif

