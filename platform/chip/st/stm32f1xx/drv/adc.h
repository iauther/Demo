#ifndef __ADC_Hx__
#define __ADC_Hx__

#include "types.h"
#include "drv/gpio.h"

#define ADC_CH      2


typedef struct {
    gpio_pin_t  pin;
    U8          usbDMA;
}adc_cfg_t;

typedef struct {
    U32         adc[ADC_CH];
}adc_data_t;


handle_t adc_init(adc_cfg_t *cfg);

int adc_deinit(handle_t *h);

U32 adc_read(handle_t h);

#endif

