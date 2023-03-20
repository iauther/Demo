#ifndef __ADC_Hx__
#define __ADC_Hx__

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

typedef void (*adc_callback_t)(U8 *data, U32 len);

typedef struct {
    ADC_TypeDef         *adc;
    gpio_pin_t          pin;
    U32                 ch;         //adc channel
    U32                 freq;
    U32                 samples;    //sample points
    
    U32                 rank;
    U32                 sigdiff;
}adc_pin_t;


typedef struct {
    ADC_TypeDef         *adc;
    IRQn_Type           IRQn;
    U32                 request;
    void                *stream;
}adc_info_t;

typedef struct {
    U8                   mode;
    U8                   dual;
    U32                  samples;
    adc_callback_t       callback;
}adc_cfg_t;


int adc_init(adc_cfg_t *cfg);

int adc_start(void);
int adc2_start(void);
int adc3_start(void);

int adc_stop(void);

int adc_get(U32 *adc);

#endif

