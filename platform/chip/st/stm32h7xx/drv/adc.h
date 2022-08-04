#ifndef __ADC_Hx__
#define __ADC_Hx__

#include "types.h"
#include "drv/gpio.h"

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


typedef struct {
    gpio_pin_t          pin;
    U32                 chn;        //adc channel
    U32                 freq;
    U32                 reso;       //resolution
    
    ADC_TypeDef         *adc;
    U32                 rank;
    U32                 sigdiff;
    U32                 polar;
}adc_pin_t;

typedef struct {
    ADC_TypeDef         *adc;
    IRQn_Type           IRQn;
    U32                 request;
    void                *stream;
}dma_para_t;

typedef struct {
    U8                   dural;
}adc_cfg_t;


int adc_init(adc_cfg_t *cfg);

int adc_start(void);
int adc_stop(void);

int adc_get(U32 *adc);

#endif

