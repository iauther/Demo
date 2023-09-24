#ifndef __DAL_ADC_Hx__
#define __DAL_ADC_Hx__

#include "types.h"


enum {
    ADC_0=0,
    ADC_1,
    
    ADC_MAX
};

enum {
    CH_0=0,
    CH_1,
    
    CH_MAX
};


typedef struct {
    U8              adc;
    U32             chn;
    U32             freq;
    rx_cb_t         callback;
}dal_adc_cfg_t;


typedef struct {
    F32 vib;
    F32 t_pt;
    F32 t_in;
    F32 vbat;
}dal_adc_data_t;


handle_t dal_adc_init(dal_adc_cfg_t *cfg);
int dal_adc_config(handle_t h, dal_adc_cfg_t *cfg);
int dal_adc_start(handle_t h);
int dal_adc_stop(handle_t h);
int dal_adc_get(dal_adc_data_t *data);
    

#endif

