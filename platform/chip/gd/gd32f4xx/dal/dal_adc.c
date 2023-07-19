#include "dal_adc.h"
#include "dal_delay.h"
#include "gd32f4xx_adc.h"
#include "cfg.h"


typedef struct {
	U8      mode;
}dal_adc_handle_t;


static int adc_dma_flag=0;

handle_t dal_adc_init(void)
{
    dal_adc_handle_t *h=calloc(1, sizeof(dal_adc_handle_t));
    if(!h) {
        return NULL;
    }
    
    
    return h;
}


//设置IO初始化
static void adc_io_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOF);
    gpio_mode_set(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_10);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
    gpio_bit_set(GPIOF,GPIO_PIN_10);//直流
    
    
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_5|GPIO_PIN_7|GPIO_PIN_8);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5|GPIO_PIN_7|GPIO_PIN_8);
    gpio_bit_set(GPIOB,GPIO_PIN_5|GPIO_PIN_7|GPIO_PIN_8);
    gpio_bit_reset(GPIOB,GPIO_PIN_5|GPIO_PIN_7|GPIO_PIN_8);//1/4V/40lsb
}


//交流直流设置
static void adc_current_set(U8 alternating)
{
    if(alternating) gpio_bit_reset(GPIOF,GPIO_PIN_10);//交流
    else gpio_bit_set(GPIOF,GPIO_PIN_10);//直流
}

//采样倍率设置
static void adc_sample_multi_set(U8 multi)
{
    if(multi & 0x01) gpio_bit_set(GPIOB,GPIO_PIN_8);
    else gpio_bit_reset(GPIOB,GPIO_PIN_8);
    
    if(multi & 0x02) gpio_bit_set(GPIOB,GPIO_PIN_7);
    else gpio_bit_reset(GPIOB,GPIO_PIN_7);
    
    if(multi & 0x04) gpio_bit_set(GPIOB,GPIO_PIN_5);
    else gpio_bit_reset(GPIOB,GPIO_PIN_5);
}




//采样速度设置
static void adc_freq_set(unsigned char freq)
{
    TIMER_PSC(TIMER1) = (U32)((0x01<<freq) - 1);
}



int dal_adc_config(handle_t h, dal_adc_cfg_t *cfg)
{
    dal_adc_handle_t *dh=(dal_adc_handle_t*)h;
    
    /* ADC_DMA_channel configuration */
    dma_single_data_parameter_struct dma_single_data_parameter;
    timer_oc_parameter_struct timer_ocintpara;
    timer_parameter_struct timer_initpara;
    
    /* enable GPIOC clock */
    rcu_periph_clock_enable(RCU_GPIOA);
    /* enable ADC clock */
    rcu_periph_clock_enable(RCU_ADC0);
    /* enable timer1 clock */
    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
    /* enable DMA clock */
    rcu_periph_clock_enable(RCU_DMA1);
    
    /* config the GPIO as analog mode */
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_1);
    
    /* TIMER1 configuration */ 
    timer_initpara.prescaler         = 0;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 99;   
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER1,&timer_initpara);
    /* CH2 configuration in PWM mode1 */
    timer_ocintpara.ocpolarity  = TIMER_OC_POLARITY_HIGH;
    timer_ocintpara.outputstate = TIMER_CCX_ENABLE;
    timer_channel_output_config(TIMER1,TIMER_CH_2,&timer_ocintpara);
    timer_channel_output_pulse_value_config(TIMER1,TIMER_CH_2,49);
    timer_channel_output_mode_config(TIMER1,TIMER_CH_2,TIMER_OC_MODE_PWM1);
    timer_channel_output_shadow_config(TIMER1,TIMER_CH_2,TIMER_OC_SHADOW_DISABLE);
    timer_enable(TIMER1);
    
    /* ADC DMA_channel configuration */
    dma_deinit(DMA1, DMA_CH0);
    
    /* initialize DMA single data mode */
    dma_single_data_parameter.periph_addr = (uint32_t)(&ADC_RDATA(ADC0));
    dma_single_data_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
//dma_single_data_parameter.memory0_addr = (uint32_t)(adc_value);
    dma_single_data_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_single_data_parameter.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_single_data_parameter.direction = DMA_PERIPH_TO_MEMORY;
//dma_single_data_parameter.number = adc_buff_max;
    dma_single_data_parameter.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH0, &dma_single_data_parameter);
    dma_channel_subperipheral_select(DMA1, DMA_CH0, DMA_SUBPERI0);
    /* enable DMA circulation mode */
    dma_circulation_enable(DMA1, DMA_CH0);
    /* enable DMA channel */
    dma_channel_enable(DMA1, DMA_CH0);
    dma_interrupt_enable(DMA1, DMA_CH0, DMA_CHXCTL_FTFIE|DMA_CHXCTL_HTFIE);//
    nvic_irq_enable(DMA1_Channel0_IRQn, 4, 0);
    
    adc_clock_config(ADC_ADCCK_HCLK_DIV5);//ADC_ADCCK_HCLK_DIV5
    //规则
//adc_channel_length_config(ADC0,ADC_REGULAR_CHANNEL,1);
//adc_regular_channel_config(ADC0,0,ADC_CHANNEL_1,ADC_SAMPLETIME_3);
//adc_external_trigger_config(ADC0,ADC_REGULAR_CHANNEL,EXTERNAL_TRIGGER_FALLING);
//adc_external_trigger_source_config(ADC0,ADC_REGULAR_CHANNEL,ADC_EXTTRIG_REGULAR_T1_CH2);
    /* ADC data alignment config */
    adc_data_alignment_config(ADC0,ADC_DATAALIGN_RIGHT);
    adc_resolution_config(ADC0,ADC_RESOLUTION_12B);
    /* ADC DMA function enable */
    adc_dma_request_after_last_enable(ADC0);
    adc_dma_mode_enable(ADC0);
    /* enable ADC interface */
    adc_enable(ADC0);
    
    /* wait for ADC stability */
    dal_delay_ms(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);
    dal_delay_ms(10);
 
    
    return 0;
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
    

    return 0;
}

int dal_adc_stop(handle_t h)
{
    dal_adc_handle_t *dh=(dal_adc_handle_t*)h;
    
   
    
    return 0;
}


int dal_adc_get(handle_t h, U32 *adc)
{
    dal_adc_handle_t *dh=(dal_adc_handle_t*)h;
    
    return 0;
}


void xDMA1_Channel0_IRQHandler(void)
{   
    if(dma_interrupt_flag_get(DMA1, DMA_CH0,DMA_INT_FLAG_HTF) == SET) adc_dma_flag=0;
    else adc_dma_flag=1;
    
    dma_interrupt_flag_clear(DMA1, DMA_CH0, DMA_INT_FLAG_FTF|DMA_INT_FLAG_HTF);
}



