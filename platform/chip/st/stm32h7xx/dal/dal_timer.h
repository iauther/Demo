#ifndef __DAL_TIMER_Hx__
#define __DAL_TIMER_Hx__

#include "types.h"

typedef void (*htimer_callback_t)(void *arg);

typedef struct {
    U8                  tmr;
}htimer_cfg_t;

typedef struct {
    int                 ms;
    int                 freq;
    int                 repeat;
    htimer_callback_t   callback;
    void                *user_data;
}htimer_set_t;


int dal_timer_init(htimer_cfg_t *cfg);
int dal_timer_deinit(void);
handle_t dal_timer_new(void);
int dal_timer_free(handle_t h);

int dal_timer_set(handle_t h, htimer_set_t *set);
int dal_timer_start(handle_t h);
int dal_timer_stop(handle_t h);


#endif
