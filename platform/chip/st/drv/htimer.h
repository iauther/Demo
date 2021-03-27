#ifndef __HTIMER_Hx__
#define __HTIMER_Hx__

#include "types.h"

typedef enum {
    HTIMER_2,
    HTIMER_3,
    HTIMER_4,
    HTIMER_5,
    HTIMER_6,
    HTIMER_7,
    
    HTIMER_MAX
}HTIMER;

typedef void (*htimer_callback_t)(TIM_HandleTypeDef *htim);

int htimer_init(HTIMER tmr, TIM_Base_InitTypeDef *init, htimer_callback_t fn);

int htimer_deinit(HTIMER tmr);

int htimer_master_confg(HTIMER tmr, TIM_MasterConfigTypeDef *cfg);

int htimer_ic_config(HTIMER tmr, U32 channel, TIM_IC_InitTypeDef *cfg);

int htimer_start(HTIMER tmr);

int htimer_stop(HTIMER tmr);

#endif
