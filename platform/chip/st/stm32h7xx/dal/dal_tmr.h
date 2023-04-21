#ifndef __DAL_TMR_Hx__
#define __DAL_TMR_Hx__

#include "types.h"

typedef struct {
    U32   freq;
}dal_tmr_cfg_t;

int dal_tmr_init(void);
int dal_tmr_config(dal_tmr_cfg_t *cfg);
int dal_tmr_start(void);
int dal_tmr_stop(void);

#endif
