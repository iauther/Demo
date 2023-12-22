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
    F32 vib;        //���ź�
    F32 t_pt;       //pt100�¶�
    F32 t_in;       //�ڲ��¶�
    F32 vref;       //�ڲ��ο���ѹ
    F32 vbat;       //��ص�ѹ
}dal_adc_data_t;


int dal_adc_init(dal_adc_cfg_t *cfg);
int dal_adc_deinit(void);
int dal_adc_start(void);
int dal_adc_stop(void);
int dal_adc_calc(U16 *val, dal_adc_data_t *data);
int dal_adc_read(dal_adc_data_t *data);

#endif

