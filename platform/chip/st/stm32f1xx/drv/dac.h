#ifndef __DAC_Hx__
#define __DAC_Hx__

#include "types.h"
#include "drv/gpio.h"

#define ADC_CH      2


typedef struct {
    gpio_pin_t  pin;
}dac_cfg_t;

typedef struct {
    U32         adc[ADC_CH];
}adc_data_t;


handle_t dac_init(dac_cfg_t *cfg);

int dac_deinit(handle_t *h);

int dac_set(handle_t h, F32 v);

#endif

