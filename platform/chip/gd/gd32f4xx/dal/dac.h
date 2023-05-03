#ifndef __DAC_Hx__
#define __DAC_Hx__

#include "dal/gpio.h"


enum {
    CH_1=1,
    CH_2,
    CH_3,
    CH_4,
    CH_5,
    CH_6,
    CH_7,
    CH_8,
    CH_9,
    CH_10,
    CH_11,
    CH_12,
    CH_13,
    CH_14,
    CH_15,
    CH_16,
    CH_17,
    CH_18,
    CH_19,
    CH_20,
    CH_21,
    CH_22,
    CH_23,
    CH_24,
    CH_25,
    CH_26,
    CH_27,
    CH_28,
    CH_29,
    CH_30,
    CH_31,
    CH_32, 
};



typedef struct {
    U8                   mode;
}dac_cfg_t;


int dac_init(void);
int dac_config(dac_cfg_t *cfg);
int dac_start(void);
int dac_stop(void);

#endif

