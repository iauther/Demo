#ifndef __DAC_Hx__
#define __DAC_Hx__

#include "types.h"

typedef struct {
    U32  freq;
    U32  points;
    U32  fdiv;
    int  enable;
}dac_param_t;


int dac_init(void);
int dac_set(dac_param_t *para);
int dac_data_fill(U16 *data, U32 cnt);
int dac_start(void);
int dac_stop(void);

#endif
