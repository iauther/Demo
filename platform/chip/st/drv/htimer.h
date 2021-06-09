#ifndef __HTIMER_Hx__
#define __HTIMER_Hx__

#include "types.h"

enum {
    HTIMER_2,
    HTIMER_3,
    HTIMER_4,
    HTIMER_5,
    HTIMER_6,
    HTIMER_7,
    
    HTIMER_MAX
};

typedef void (*htimer_callback_t)(void *user_data);

typedef struct {
    int                 ms;
    int                 freq;
    int                 repeat;
    htimer_callback_t   callback;
    void                *user_data;
}htimer_set_t;

/*
int htimer_init(HTIMER tmr, TIM_Base_InitTypeDef *init, htimer_callback_t fn);
int htimer_deinit(HTIMER tmr);
int htimer_master_confg(HTIMER tmr, TIM_MasterConfigTypeDef *cfg);
int htimer_ic_config(HTIMER tmr, U32 channel, TIM_IC_InitTypeDef *cfg);
int htimer_start(HTIMER tmr);
int htimer_stop(HTIMER tmr);
*/

int htimer_init(U8 port);
handle_t htimer_new(void);
int htimer_free(handle_t *h);
int htimer_get(handle_t h, htimer_set_t *set);
int htimer_set(handle_t h, htimer_set_t *set);
int htimer_start(handle_t h);
int htimer_stop(handle_t h);

handle_t htimer_sw_start(htimer_set_t *set);
int htimer_sw_stop(handle_t *h);

#endif
