#ifndef __HTIMER_Hx__
#define __HTIMER_Hx__

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


int htimer_init(htimer_cfg_t *cfg);
int htimer_deinit(void);
handle_t htimer_new(void);
int htimer_free(handle_t h);

int htimer_set(handle_t h, htimer_set_t *set);
int htimer_start(handle_t h);
int htimer_stop(handle_t h);


#endif
