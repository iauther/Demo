#ifndef __ADC_Hx__
#define __ADC_Hx__

#include "dal/gpio.h"

enum {
    ADC_1=0,
    ADC_2,
    ADC_3,
    
    ADC_MAX
};

enum {
    POLAR_P=0,
    POLAR_N,
};

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
    gpio_pin_t          pin;
    U32                 ch;         //adc channel
    U32                 freq;
    U32                 reso;       //resolution
    
    U32                 rank;
    U32                 sigdiff;
    U32                 polar;
}adc_pin_t;


typedef struct {
    ADC_TypeDef         *adc;
    IRQn_Type           IRQn;
    U32                 request;
    void                *stream;
}adc_info_t;

typedef struct {
    U8                   dural;
}adc_cfg_t;


int adc_init(void);

int adc_start(void);
int adc2_start(void);
int adc3_start(void);

int adc_stop(void);

int adc_get(U32 *adc);

#endif

