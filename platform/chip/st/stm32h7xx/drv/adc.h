#ifndef __ADC_Hx__
#define __ADC_Hx__

#include "types.h"
#include "drv/gpio.h"

enum {
    ADC_1=0,
    ADC_2,
    ADC_1_2,
    ADC_3,
};



int adc_init(gpio_pin_t *pin);

int adc_start(void);
int adc_stop(void);

int adc_get(U32 *adc);

#endif

