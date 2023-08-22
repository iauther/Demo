#ifndef __DAL_ADC_Hx__
#define __DAL_ADC_Hx__

#include "dal_gpio.h"


enum {
    CH_0=0,
    CH_1,
    
    CH_MAX
};

typedef void (*dal_adc_callback_t)(U8 *data, U32 len);



typedef struct {
    U8                   mode;
    U8                   dual;
    U32                  samples;
    dal_adc_callback_t   callback;
}dal_adc_cfg_t;


handle_t dal_adc_init(void);
int dal_adc_config(handle_t h, dal_adc_cfg_t *cfg);
int dal_adc_start(handle_t h);
int dal_adc_stop(handle_t h);
int dal_adc_get(handle_t h, U32 *adc);

#endif

