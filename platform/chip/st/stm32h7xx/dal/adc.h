#ifndef __ADC_Hx__
#define __ADC_Hx__

#include "dal/gpio.h"


enum {
    CH0=0,
    CH1,
    CH2,
    CH3,
    CH4,
    CH5,
    CH6,
    CH7,
    CH8,
    CH9,
    CH10,
    CH11,
    CH12,
    CH13,
    CH14,
    CH15,
    
    CHMAX,
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


int adc_init(void);
int adc_config(adc_cfg_t *cfg);
int adc_start(void);
int adc2_start(void);
int adc3_start(void);

int adc_stop(void);

int adc_get(U32 *adc);

#endif

