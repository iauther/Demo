#ifndef __TMR_Hx__
#define __TMR_Hx__

#include "types.h"

typedef struct {
    U32   freq;
}tmr_cfg_t;

int tmr_init(void);
int tmr_config(tmr_cfg_t *cfg);
int tmr_start(void);
int tmr_stop(void);

#endif
