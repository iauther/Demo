#ifndef __DAL_TIMER_Hx__
#define __DAL_TIMER_Hx__

#include "types.h"


enum {
    TIMER_1=0,
    TIMER_2,
    TIMER_3,
    TIMER_4,
    TIMER_5,
    TIMER_6,
    
    TIMER_MAX
};


typedef void (*timer_callback_t)(handle_t h, void *arg);

typedef struct {
    U8                  timer;
    int                 freq;       //hz
    int                 times;
    void                *arg;
    timer_callback_t    callback;
}dal_timer_cfg_t;


handle_t dal_timer_init(dal_timer_cfg_t *cfg);
int dal_timer_deinit(handle_t h);
int dal_timer_set(handle_t h, dal_timer_cfg_t *cfg);
int dal_timer_start(handle_t h);
int dal_timer_stop(handle_t h);

#endif

