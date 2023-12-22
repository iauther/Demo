#include "dal_adc.h"
#include "dal_delay.h"
#include "gd32f4xx_adc.h"
#include "dal_gpio.h"
#include "log.h"
#include "cfg.h"

//#define USE_TIMER
#define USE_DMA


typedef struct {
	U8              mode;
    U16             value[5];
    
    dal_adc_cfg_t   cfg;
}dal_adc_handle_t;

typedef struct {
	dal_gpio_t io;
    
    
}adc_pin_info_t;



/*
    振动通道 PC4 ADC_CH4
    温度通道 PA4 ADC_CH14

    ADC0
    片内温度 ADC_CH16
    基准电压 ADC_CH17
    电池电压 ADC_CH18
*/

static dal_adc_handle_t adcHandle;
static void adc_calc(U16 *val, dal_adc_data_t *data)
{
    F32 vref,lsb;
    dal_adc_handle_t *h=&adcHandle;
    
    /*
        val[0]:振动, val[1]:pt100, val[2]:内部温度, val[3]:参考电压, val[4]:电池电压
    
        内部温度(°C) = {(V25 – Vtemperature) / Avg_Slope} + 25
            V25：内部温度传感器在25°C下的电压，典型值请参考相关型号datasheet
            Avg_Slope：温度与内部温度传感器输出电压曲线的均值斜率，典型值请参考相关型号datasheet
        
    */
    lsb = 3.3f/4096;
    
    data->vib  = val[0];
    data->t_pt = val[1]*lsb;
    data->t_in = (1.43f - val[2]*lsb)*1000/4.3f + 25;
    data->vref = val[3]*lsb;
    data->vbat = (val[4]*lsb)*4;
}


static void rcu_config(dal_adc_handle_t *h)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_ADC0);
    
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
    rcu_periph_clock_enable(RCU_TIMER0);

    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);     //APB2(120MHz) -> PCLK2/8 = 15MHz 
}
static void gpio_config(void)
{
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);
}
void DMA1_Channel0_IRQHandler(void)
{
    dal_adc_handle_t *h=&adcHandle;
    
    if(dma_interrupt_flag_get(DMA1, DMA_CH0, DMA_INT_FLAG_HTF)) {
        dma_interrupt_flag_clear(DMA1, DMA_CH0, DMA_INT_FLAG_HTF);

        if(h && h->cfg.callback) {
            h->cfg.callback(h->value, 5);
        }
    }
    else if(dma_interrupt_flag_get(DMA1, DMA_CH0, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA1, DMA_CH0, DMA_INT_FLAG_FTF);
        
        if(h && h->cfg.callback) {
            h->cfg.callback(h->value, 5);
        }
    }

}


#define DMA_INT_FLAGS    DMA_CHXCTL_HTFIE|DMA_CHXCTL_FTFIE
static void dma_config(dal_adc_handle_t *h)
{
    dma_single_data_parameter_struct para;
    
    rcu_periph_clock_enable(RCU_DMA1);
    
    dma_deinit(DMA1, DMA_CH0);
    para.periph_addr = (U32)(&ADC_RDATA(ADC0));
    para.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    para.memory0_addr = (U32)(h->value);
    para.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    para.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    para.direction = DMA_PERIPH_TO_MEMORY;
    para.number = sizeof(h->value)/sizeof(h->value[0]);
    para.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH0, &para);
    
    dma_channel_subperipheral_select(DMA1, DMA_CH0, DMA_SUBPERI0);

    dma_circulation_enable(DMA1, DMA_CH0);
    dma_channel_enable(DMA1, DMA_CH0);
    
    //dma_interrupt_enable(DMA1, DMA_CH0, DMA_INT_FLAGS);
    
}
static void adc_config(dal_adc_handle_t *h)
{
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);
    adc_resolution_config(ADC0, ADC_RESOLUTION_12B);

    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, sizeof(h->value)/sizeof(h->value[0]));
    
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_4,   ADC_SAMPLETIME_15);        //振动信号
    adc_regular_channel_config(ADC0, 1, ADC_CHANNEL_14,  ADC_SAMPLETIME_15);        //pt1000信号
    adc_regular_channel_config(ADC0, 2, ADC_CHANNEL_16,  ADC_SAMPLETIME_15);        //温度，采样时间至少设置为ts_temp us
    adc_regular_channel_config(ADC0, 3, ADC_CHANNEL_17,  ADC_SAMPLETIME_15);        //内部参考电压
    adc_regular_channel_config(ADC0, 4, ADC_CHANNEL_18,  ADC_SAMPLETIME_15);        //电池电压
    
    adc_channel_16_to_18(ADC_VBAT_CHANNEL_SWITCH,ENABLE);
    adc_channel_16_to_18(ADC_TEMP_VREF_CHANNEL_SWITCH,ENABLE);
    
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_T0_CH0); 
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    
    adc_dma_request_after_last_enable(ADC0);
    adc_dma_mode_enable(ADC0);
    
    adc_enable(ADC0);
    
    /* wait for ADC stability */
    dal_delay_ms(1);
    
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);
    
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
    
}
void timer_config(dal_adc_handle_t *h)
{
    timer_oc_parameter_struct ocp;
    timer_parameter_struct tp;

    /* TIMER0 configuration */
    //APB2 max:120MHz
    tp.prescaler         = 119;
    tp.alignedmode       = TIMER_COUNTER_EDGE;
    tp.counterdirection  = TIMER_COUNTER_UP;
    tp.period            = 1000000/h->cfg.freq;
    tp.clockdivision     = TIMER_CKDIV_DIV1;
    tp.repetitioncounter = 0;
    timer_init(TIMER0, &tp);
    
    timer_primary_output_config(TIMER0, ENABLE);
    timer_dma_enable(TIMER0, TIMER_DMA_CH0D);
    
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_T0_CH0); 
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, EXTERNAL_TRIGGER_RISING);
    //adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    
    /* enable TIMER1 */
    timer_enable(TIMER0);
}


int dal_adc_init(dal_adc_cfg_t *cfg)
{
    dal_adc_handle_t *h=&adcHandle;
    
    if(cfg) {
        h->cfg = *cfg;
    }
    
    rcu_config(h);
    gpio_config();

#ifdef USE_DMA
    dma_config(h);
#endif
    
#ifdef USE_TIMER
    timer_config(h);
#endif
    
    adc_config(h);
    
    return 0;
}


int dal_adc_deinit(void)
{
    dal_adc_handle_t *h=&adcHandle;
    
    
    return 0;
}


int dal_adc_start(void)
{
    dal_adc_handle_t *h=&adcHandle;
    adc_enable(ADC0);
    
    return 0;
}


int dal_adc_stop(void)
{
    dal_adc_handle_t *h=&adcHandle;
    
    adc_disable(ADC0);
    
    return 0;
}


int dal_adc_calc(U16 *val, dal_adc_data_t *data)
{
    adc_calc(val, data);
    
    return 0;
}


static U16 get_adc(U8 ch)
{
    adc_regular_channel_config(ADC0, 0, ch, ADC_SAMPLETIME_15);
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
    
    while(!adc_flag_get(ADC0, ADC_FLAG_EOC));
    adc_flag_clear(ADC0, ADC_FLAG_EOC);
    
    adc_regular_data_read(ADC0);
    
    return 0;
}


int dal_adc_read(dal_adc_data_t *data)
{
    dal_adc_handle_t *h=&adcHandle;
    
	//adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
#ifdef USE_DMA
    adc_calc(h->value, data);
#else
    
#endif
    
	return 0;
}

