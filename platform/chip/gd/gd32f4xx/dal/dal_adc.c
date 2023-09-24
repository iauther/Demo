#include "dal_adc.h"
#include "dal_delay.h"
#include "gd32f4xx_adc.h"
#include "dal_gpio.h"
#include "cfg.h"


typedef struct {
	U8      mode;
    
    
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

       
static U16 adc_value[5];
static void rcu_config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_ADC0);
    
    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
    
    rcu_periph_clock_enable(RCU_DMA1);
    
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);     //APB2(120MHz) -> PCLK2 -> 
}
static void gpio_config(void)
{
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);
}
static void dma_config(void)
{
    dma_single_data_parameter_struct para;
    dma_deinit(DMA1, DMA_CH0);

    para.periph_addr = (U32)(&ADC_RDATA(ADC0));
    para.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    para.memory0_addr = (U32)(adc_value);
    para.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    para.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    para.direction = DMA_PERIPH_TO_MEMORY;
    para.number = sizeof(adc_value)/sizeof(adc_value[0]);
    para.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH0, &para);
    dma_channel_subperipheral_select(DMA1, DMA_CH0, DMA_SUBPERI0);

    dma_circulation_enable(DMA1, DMA_CH0);
    dma_channel_enable(DMA1, DMA_CH0);
}
static void adc_config(void)
{
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 5);
    
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_4,   ADC_SAMPLETIME_15);
    adc_regular_channel_config(ADC0, 1, ADC_CHANNEL_14,  ADC_SAMPLETIME_15);
    adc_regular_channel_config(ADC0, 2, ADC_CHANNEL_16,  ADC_SAMPLETIME_15);
    adc_regular_channel_config(ADC0, 3, ADC_CHANNEL_17,  ADC_SAMPLETIME_15);
    adc_regular_channel_config(ADC0, 4, ADC_CHANNEL_18,  ADC_SAMPLETIME_15);
    
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_T0_CH0); 
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    

    /* ADC DMA function enable */
    adc_dma_request_after_last_enable(ADC0);
    adc_dma_mode_enable(ADC0);

    adc_enable(ADC0);
    
    /* wait for ADC stability */
    dal_delay_ms(1);
    
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);

    /* enable ADC software trigger */
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}
void timer_config(void)
{
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;

    /* TIMER0 configuration */
    timer_initpara.prescaler         = 19999;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 9999;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER0, &timer_initpara);

    /* CH0 configuration in PWM mode0 */
    timer_ocintpara.ocpolarity  = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
    timer_ocintpara.outputnstate = TIMER_CCXN_DISABLE;
    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
    timer_ocintpara.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
    timer_ocintpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER0,TIMER_CH_0,&timer_ocintpara);

    timer_channel_output_pulse_value_config(TIMER1,TIMER_CH_1,3999);
    timer_channel_output_mode_config(TIMER1,TIMER_CH_1,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER1,TIMER_CH_1,TIMER_OC_SHADOW_DISABLE);
    
    /* enable TIMER1 */
    timer_enable(TIMER0);
}


handle_t dal_adc_init(dal_adc_cfg_t *cfg)
{
    dal_adc_handle_t *h=calloc(1, sizeof(dal_adc_handle_t));
    if(!h) {
        return NULL;
    }
    
    rcu_config();
    gpio_config();
    dma_config();
    adc_config();
    
    return h;
}


int dal_adc_deinit(handle_t h)
{
    dal_adc_handle_t *dh=(dal_adc_handle_t*)h;
    
    
    free(h);
    
    return 0;
}



int dal_adc_start(handle_t h)
{
    dal_adc_handle_t *dh=(dal_adc_handle_t*)h;
    
    adc_enable(ADC0);
    
    return 0;
}

int dal_adc_stop(handle_t h)
{
    dal_adc_handle_t *dh=(dal_adc_handle_t*)h;
    
    adc_disable(ADC0);
    
    return 0;
}


int dal_adc_get(dal_adc_data_t *data)
{
    F32 vref;
    
    vref = (adc_value[3] * 3.3f / 4096);
    data->t_in = (1.43f - adc_value[2]*3.3f/4096) * 1000 / 4.3f + 25;
    data->vbat = (adc_value[4] * 3.3f / 4096) * 4;
    
    return 0;
}




