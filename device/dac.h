#ifndef __DAC_Hx__
#define __DAC_Hx__

#include "dal_dac.h"

typedef struct {
    U32  freq;
    U32  fdiv;
    int  enable;
    
    buf_t buf;
    dac_fn fn;
}dac_param_t;


int dac_init(dac_param_t *para);
int dac_set(dac_param_t *para);
int dac_data_fill(U16 *data, U32 cnt);
int dac_start(void);
int dac_stop(void);

#endif
