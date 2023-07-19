#ifndef __DAL_DAC_Hx__
#define __DAL_DAC_Hx__

#include "dal_gpio.h"

enum {
    DAC_0=0,
    DAC_1,
    
    DAC_MAX
};



typedef struct {
    U8      port;
    U32     freq;
    
    void    *buf;
    int     bufLen;
}dal_dac_cfg_t;


handle_t dal_dac_init(dal_dac_cfg_t *cfg);
int dal_dac_start(handle_t h);
int dal_dac_stop(handle_t h);

#endif

