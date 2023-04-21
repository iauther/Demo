#ifndef __DAL_ADC_Hx__
#define __DAL_ADC_Hx__

#include "dal_gpio.h"


enum {
    CH_0=0,
    CH_1,
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
    
    CH_MAX,
};

typedef void (*dal_adc_callback_t)(U8 *data, U32 len);

typedef struct {
    ADC_TypeDef         *adc;
    dal_gpio_pin_t          pin;
    U32                 ch;         //adc channel
    U32                 freq;
    U32                 samples;    //sample points
    
    U32                 rank;
    U32                 sigdiff;
}dal_adc_pin_t;


typedef struct {
    ADC_TypeDef         *adc;
    IRQn_Type           IRQn;
    U32                 request;
    void                *stream;
}dal_adc_info_t;

typedef struct {
    U8                   mode;
    U8                   dual;
    U32                  samples;
    dal_adc_callback_t   callback;
}dal_adc_cfg_t;


int dal_adc_init(void);
int dal_adc_config(dal_adc_cfg_t *cfg);
int dal_adc_start(void);
int dal_adc_stop(void);
int dal_adc_get(U32 *adc);

#endif

