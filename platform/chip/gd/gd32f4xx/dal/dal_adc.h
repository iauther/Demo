#ifndef __DAL_ADC_Hx__
#define __DAL_ADC_Hx__

#include "types.h"


enum {
    ADC_0=0,
    ADC_1,
    
    ADC_MAX
};


typedef void (*adc_callback_t)(U16 *data, U32 cnt);

typedef struct {
    U8              adc;
    U32             chn;
    U32             freq;
    adc_callback_t  callback;
}dal_adc_cfg_t;


typedef struct {
    F32 vib;        //振动信号
    F32 t_pt;       //pt100温度
    F32 t_in;       //内部温度
    F32 vref;       //内部参考电压
    F32 vbat;       //电池电压
}dal_adc_data_t;


int dal_adc_init(dal_adc_cfg_t *cfg);
int dal_adc_deinit(void);
int dal_adc_start(void);
int dal_adc_stop(void);
int dal_adc_calc(U16 *val, dal_adc_data_t *data);
int dal_adc_read(dal_adc_data_t *data);

#endif

