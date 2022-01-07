#ifndef __ADC_Hx__
#define __ADC_Hx__

#include "types.h"
#include "drv/gpio.h"

int adc_init(gpio_pin_t *pin);

int adc_get(U32 *adc);

#endif

