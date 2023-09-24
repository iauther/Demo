#ifndef __DAL_DAC_Hx__
#define __DAL_DAC_Hx__

#include "dal_gpio.h"

enum {
    DAC_0=0,
    DAC_1,
    
    DAC_MAX
};


typedef struct {
    U32     freq;
    void    *buf;
    int     blen;
}dal_dac_para_t;


handle_t dal_dac_init(U8 port);
int dal_dac_set(handle_t h, dal_dac_para_t *para);
int dal_dac_start(handle_t h);
int dal_dac_stop(handle_t h);

#endif

