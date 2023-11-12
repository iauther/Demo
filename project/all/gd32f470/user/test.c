#include "log.h"
#include "rs485.h"
#include "ecxxx.h"
#include "ads9120.h"
#include "dal_dac.h"
#include "dal_pwm.h"
#include "dal_uart.h"
#include "dal_delay.h"
#include "dal_timer.h"
#include "dal_nor.h"
#include "power.h"
#include "ecxxx.h"
#include "dal.h"
#include "rtc.h"
#include "mem.h"
#include "sflash.h"
#include "upgrade.h"
#include "fs.h"
#include "cfg.h"
#include "list.h"
#include "dsp.h"


#ifndef OS_KERNEL
/*
  gd32f470内部资源：  240MHz, 3072KB flash, 768KB sram
                     3*12bit 2.6 MSPS ADCs, 2*12bit DACs
                     8*16bit general timers, 2*16-bit PWM timers,
                     2*32bit general timers, 2*16bit basic timers
                     6*SPIs, 3*I2Cs, 4*USARTs 4*UARTs, 2*I2Ss
                     2*CANs, 1*SDIO1*USBFS 1*USBHS, 1*ENET
                     
                     
时钟源：

    CK_SYS:      系统时钟, 包含3个时钟源：
                 IRC16M 内部16MHz （默认）
                 HXTAL 外部高速时钟
                 PLLP 倍频时钟P
    HCLK：       AHB bus,Cortex-M4,SRAM,DMA,peripherals, max:240MHz
    PCLK1：      APB1 peripherals, max:60MHz
    PCLK2：      APB2 peripherals, max:120MHz
    CKTIMERx：   TIMER1,2,3,4,5,6,11,12,13
    CKTIMERx：   TIMER0,7,8,9,10
    CK_ADCX：    ADC 0,1,2                   
*/

typedef struct {
	U8              mode;
    U16             value[5];
    
    dal_adc_cfg_t   cfg;
}dal_adc_handle_t;
static dal_adc_handle_t adcHandle;
static void adc_calc(U16 *val, dal_adc_data_t *data)
{
    F32 vref,lsb;
    
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


void rcu_config(void)
{
    /* enable GPIOC clock */
    rcu_periph_clock_enable(RCU_GPIOA);
    /* enable ADC clock */
    rcu_periph_clock_enable(RCU_ADC0);
    /* enable timer1 clock */
    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
    /* enable DMA clock */
    rcu_periph_clock_enable(RCU_DMA1);
    /* config ADC clock */
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
}


void gpio_config(void)
{
    /* config the GPIO as analog mode */
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);
    
    
    //gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_1);
    //gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_2);
    //gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_3);
    //gpio_mode_set(ADC_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,ADC_GPIO_PIN );
}

/*!
    \brief      configure the DMA peripheral
    \param[in]  none
    \param[out] none
    \retval     none
*/
void dma_config(void)
{
    dal_adc_handle_t *h=&adcHandle;
    
    /* ADC_DMA_channel configuration */
    dma_single_data_parameter_struct dma_single_data_parameter;

    /* ADC DMA_channel configuration */
    dma_deinit(DMA1, DMA_CH0);

    /* initialize DMA single data mode */
    dma_single_data_parameter.periph_addr = (uint32_t)(&ADC_RDATA(ADC0));
    dma_single_data_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_single_data_parameter.memory0_addr = (uint32_t)(h->value);
    dma_single_data_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_single_data_parameter.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    dma_single_data_parameter.direction = DMA_PERIPH_TO_MEMORY;
    dma_single_data_parameter.number = sizeof(h->value)/sizeof(h->value[0]);
    dma_single_data_parameter.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA1, DMA_CH0, &dma_single_data_parameter);
    dma_channel_subperipheral_select(DMA1, DMA_CH0, DMA_SUBPERI0);

    /* enable DMA circulation mode */
    dma_circulation_enable(DMA1, DMA_CH0);

    /* enable DMA channel */
    dma_channel_enable(DMA1, DMA_CH0);
}


void adc_config(void)
{
    dal_adc_handle_t *h=&adcHandle;
    
    /* ADC mode config */
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    /* ADC contineous function disable */
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    /* ADC scan mode disable */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    /* ADC data alignment config */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    /* ADC channel length config */
    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, sizeof(h->value)/sizeof(h->value[0]));
    /* ADC routine channel config */
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_4,  ADC_SAMPLETIME_15);
    adc_regular_channel_config(ADC0, 1, ADC_CHANNEL_14, ADC_SAMPLETIME_15);
    adc_regular_channel_config(ADC0, 2, ADC_CHANNEL_16, ADC_SAMPLETIME_15);
    adc_regular_channel_config(ADC0, 3, ADC_CHANNEL_17, ADC_SAMPLETIME_15);
    adc_regular_channel_config(ADC0, 4, ADC_CHANNEL_18, ADC_SAMPLETIME_15);
    
    /* ADC trigger config */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_T0_CH0); 
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, EXTERNAL_TRIGGER_DISABLE);

    adc_channel_16_to_18(ADC_VBAT_CHANNEL_SWITCH,ENABLE);
    adc_channel_16_to_18(ADC_TEMP_VREF_CHANNEL_SWITCH,ENABLE);

    /* ADC DMA function enable */
    adc_dma_request_after_last_enable(ADC0);
    adc_dma_mode_enable(ADC0);

    /* enable ADC interface */
    adc_enable(ADC0);
    /* wait for ADC stability */
    dal_delay_ms(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);

    /* enable ADC software trigger */
    adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}
static void adc_test(void)
{
    dal_adc_handle_t *h=&adcHandle;
    dal_adc_data_t dat;
    
    rcu_config();
    gpio_config();
    dma_config();
    adc_config();
    while(1) {
        adc_calc(h->value, &dat);
        dal_delay_ms(1000);
    }
}



void test_main(void)
{
    int i,r,cnt=0x7a;
    U8 reg=0x00;
    U8 tmp[0x7a];
    
    mem_init();
    log_init(NULL);
    
    while(1) {
        r = rtc2_dump(reg, tmp, cnt);
        if(r==0) {
            for(i=0; i<cnt; i++) {
                LOGD("reg[0x%02x]: 0x%02x\n", i, tmp[i]);
            }
            LOGD("\n");
        }
        else {
            LOGD("rtc dump failed\n");
        }
        
        dal_delay_ms(1000);
    }
}

#endif

