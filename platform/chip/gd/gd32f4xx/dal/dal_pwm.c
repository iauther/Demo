#include "dal_pwm.h"
#include "log.h"
#include "cfg.h"
#include "dal_delay.h"
#include "gd32f4xx_timer.h"
#include "gd32f4xx_gpio.h"

//PB5  <--->  CONVST    TIMER2_CH1
//PB4  <--->  CS        TIMER2_CH0

//#define USE_TIMER0


#define TMP_CNT  8
static U16 tmpData[TMP_CNT]={0};
static void tmr_dma_init(U32 dma, dma_channel_enum dmaChn, dma_subperipheral_enum subChn, U8 IRQn, void *buf, U32 cnt);


//主从timer设置
void ms_timer_config(void)
{
    timer_parameter_struct timer_initpara;

    timer_master_slave_mode_config(TIMER2, TIMER_MASTER_SLAVE_MODE_ENABLE);
    timer_master_output_trigger_source_select(TIMER2, TIMER_TRI_OUT_SRC_UPDATE);
    
    timer_deinit(TIMER0);
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 0;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 3;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER0, &timer_initpara);
    timer_auto_reload_shadow_enable(TIMER0);

    /* 从模式选择:外部时钟模式0 */
    timer_slave_mode_select(TIMER0, TIMER_SLAVE_MODE_EXTERNAL0);
    
    /* 选择定时器输入触发源:内部触发2(ITI2) */
    timer_input_trigger_source_select(TIMER0, TIMER_SMCFG_TRGSEL_ITI2);
    
    timer_enable(TIMER0);
}

//TIMER2产生PWM，TIMER0捕获PWM下降沿
static void timer0_cap_config(void)
{
	timer_ic_parameter_struct ic;
    timer_parameter_struct    init;
    
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_8);
	gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_8);
    
    
	rcu_periph_clock_enable(RCU_TIMER0);
    timer_deinit(TIMER0);
    
	init.prescaler = 99;
    init.period = 99;
	init.alignedmode = TIMER_COUNTER_EDGE;
	init.counterdirection = TIMER_COUNTER_UP;
	init.clockdivision = TIMER_CKDIV_DIV1;
	init.repetitioncounter = 0;
	timer_init(TIMER0, &init);
	
	ic.icpolarity  = TIMER_IC_POLARITY_FALLING;
	ic.icselection = TIMER_IC_SELECTION_DIRECTTI;
	ic.icprescaler = TIMER_IC_PSC_DIV1;
	ic.icfilter = 0;
	
    //PA8:TIMER0_CH0  PA9:TIMER0_CH1
	timer_input_capture_config(TIMER0,TIMER_CH_0, &ic);
    timer_auto_reload_shadow_enable(TIMER0);
    
    //timer_interrupt_flag_clear(TIMER0,TIMER_INT_CH0);
    //timer_interrupt_enable(TIMER0,TIMER_INT_CH0);
    //nvic_irq_enable(TIMER0_Channel_IRQn, 3, 0);
    
	timer_dma_enable(TIMER0, TIMER_DMA_CH0D);
    
    tmr_dma_init(DMA1, DMA_CH1, DMA_SUBPERI6, DMA1_Channel1_IRQn, tmpData, TMP_CNT);
}
void DMA1_Channel1_IRQHandler(void)
{
#if 0
    if(dma_interrupt_flag_get(DMA0, DMA_CH4, DMA_INT_FLAG_FTF)){
        dma_interrupt_flag_clear(DMA0, DMA_CH4, DMA_INT_FLAG_FTF);
    
        dma_channel_disable(DMA0, DMA_CH5);
        dma_memory_address_config(DMA0, DMA_CH4, DMA_MEMORY_0, (U32)tmpData);
        dma_transfer_number_config(DMA0, DMA_CH4, TMP_CNT);
        
        dma_channel_enable(DMA0, DMA_CH4);
    }
#endif
}

static void timer0_config(void)
{
	timer_ic_parameter_struct ic;
    timer_parameter_struct    init;
    
	rcu_periph_clock_enable(RCU_TIMER0);
    timer_deinit(TIMER0);
    
	init.prescaler = 99;
    init.period = 99;
	init.alignedmode = TIMER_COUNTER_EDGE;
	init.counterdirection = TIMER_COUNTER_UP;
	init.clockdivision = TIMER_CKDIV_DIV1;
	init.repetitioncounter = 0;
	timer_init(TIMER0, &init);
	
	//timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_CH0);
    //timer_interrupt_enable(TIMER0, TIMER_INT_FLAG_CH0);
    //timer_dma_enable(TIMER0, TIMER_DMA_CH0D);
    //tmr_dma_init(DMA1, DMA_CH1, DMA_SUBPERI6, DMA1_Channel1_IRQn, tmpData, TMP_CNT);
    
    timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_TRG);
    timer_interrupt_enable(TIMER0, TIMER_INT_FLAG_TRG);
    timer_dma_enable(TIMER0, TIMER_DMA_TRGD);
    
    tmr_dma_init(DMA1, DMA_CH0, DMA_SUBPERI6, DMA1_Channel0_IRQn, tmpData, TMP_CNT);
}
void DMA1_Channel0_IRQHandler(void)
{
#if 0  
    if(dma_interrupt_flag_get(DMA0, DMA_CH4, DMA_INT_FLAG_FTF)){
        dma_interrupt_flag_clear(DMA0, DMA_CH4, DMA_INT_FLAG_FTF);
    
        dma_channel_disable(DMA0, DMA_CH5);
        dma_memory_address_config(DMA0, DMA_CH4, DMA_MEMORY_0, (U32)tmpData);
        dma_transfer_number_config(DMA0, DMA_CH4, TMP_CNT);
        
        dma_channel_enable(DMA0, DMA_CH4);
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////
#define DMA_INT_FLAGS    DMA_CHXCTL_HTFIE|DMA_CHXCTL_FTFIE
//#define DMA_INT_FLAGS    (DMA_CHXCTL_SDEIE|DMA_CHXCTL_TAEIE|DMA_CHXCTL_HTFIE|DMA_CHXCTL_FTFIE)
static void dma_int_flag_handle(uint32_t dma, dma_channel_enum chn)
{
    dma_channel_disable(dma, chn);
    
    if(dma_interrupt_flag_get(dma, chn, DMA_INT_FLAG_HTF)) {
        dma_interrupt_flag_clear(dma, chn, DMA_INT_FLAG_HTF);
    }
    else if(dma_interrupt_flag_get(dma, chn, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(dma, chn, DMA_INT_FLAG_FTF);
    }
    else if(dma_interrupt_flag_get(dma, chn, DMA_INT_FLAG_SDE)) {
        dma_interrupt_flag_clear(dma, chn, DMA_INT_FLAG_SDE);
    }
    else if(dma_interrupt_flag_get(dma, chn, DMA_INT_FLAG_TAE)) {
        dma_interrupt_flag_clear(dma, chn, DMA_INT_FLAG_TAE);
    }
    else if(dma_interrupt_flag_get(dma, chn, DMA_INT_FLAG_FEE)) {
        dma_interrupt_flag_clear(dma, chn, DMA_INT_FLAG_FEE);
    }
    
    dma_channel_enable(dma, chn);
}



static void rcu_config(void)
{
    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMA1);
    rcu_periph_clock_enable(RCU_GPIOB);
}
static void gpio_config(void)
{
    U32 pin=GPIO_PIN_4|GPIO_PIN_5;
    
    gpio_af_set(GPIOB, GPIO_AF_2, pin);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, pin);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, pin);
}



static void tmr_dma_init(U32 dma, dma_channel_enum dmaChn, dma_subperipheral_enum subChn, U8 IRQn, void *buf, U32 cnt)
{
    dma_single_data_parameter_struct init;
    dma_multi_data_parameter_struct  init2;
    
    dma_deinit(dma, dmaChn);
    dma_single_data_para_struct_init(&init);
   
    init.direction = DMA_MEMORY_TO_PERIPH;
    init.memory0_addr = (U32)buf;
    init.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    init.periph_memory_width = DMA_MEMORY_WIDTH_16BIT;
    init.number = cnt;
    init.periph_addr = (U32)&SPI_DATA(SPI5);
    init.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    init.priority = DMA_PRIORITY_ULTRA_HIGH;
    //init.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    init.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    
    
    init2.periph_addr = (U32)&SPI_DATA(SPI5);;
    init2.periph_width = DMA_PERIPH_WIDTH_16BIT;
    init2.memory_width = DMA_MEMORY_WIDTH_16BIT;
    init2.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    init2.memory0_addr = (U32)buf;
    init2.number = cnt;
    init2.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    init2.memory_burst_width = DMA_MEMORY_BURST_4_BEAT;
    init2.periph_burst_width = DMA_MEMORY_BURST_4_BEAT;//DMA_PERIPH_BURST_SINGLE;
    init2.critical_value = DMA_FIFO_2_WORD;
    //init2.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    init2.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    init2.direction = DMA_MEMORY_TO_PERIPH;
    init2.priority = DMA_PRIORITY_ULTRA_HIGH;
    
    dma_multi_data_mode_init(dma, dmaChn, &init2);
    //dma_single_data_mode_init(dma, dmaChn, &init);
    
    dma_channel_subperipheral_select(dma, dmaChn, subChn);
    dma_flow_controller_config(dma, dmaChn, DMA_FLOW_CONTROLLER_DMA);
    //dma_flow_controller_config(dma, dmaChn, DMA_FLOW_CONTROLLER_PERI);
    
    dma_interrupt_enable(dma, dmaChn, DMA_INT_FLAGS);
    //nvic_irq_enable(IRQn, 3, 0);
    
    dma_channel_enable(dma, dmaChn);
}

static void dma_config(void)
{
    tmr_dma_init(DMA0, DMA_CH5, DMA_SUBPERI5, DMA0_Channel5_IRQn, tmpData, TMP_CNT);
    //tmr_dma_init(DMA0, DMA_CH7, DMA_SUBPERI5, DMA0_Channel7_IRQn, tmpData, TMP_CNT);
}


typedef struct {
    U32     period;
    U32     pulse;
}pwm_tmr_para_t;
static int cal_tmr_para(U32 freq, pwm_tmr_para_t *para)
{
    int find=0;
    U32 utime;
    U32 freqMax=2500*KHZ;
    U32 pclk=SystemCoreClock/2;
    U32 period,pulse;
    U32 htime,ltime;
    
    if(freq>freqMax) {
        LOGE("freq %d is overflow, the freqMax is %d\n", freq, freqMax);
        return -1;
    }
    
    period = pclk/freq;
    for(pulse=1; pulse<period; pulse++) {

        utime = 1000000000/freq;        //ns
        htime = pulse*utime;
        ltime = (period-pulse)*utime;
        if(htime>30 && ltime>200) {
            if(para) {
                para->period = period;
                para->pulse  = pulse;
            }
            return 0;
        }
    }
    
    return -1;
}

static int timer_config(pwm_tmr_para_t *para)
{
    /* -----------------------------------------------------------------------
    TIMER CLK = 240M/10 = 24 MHz
    ----------------------------------------------------------------------- */
    int r;
    timer_oc_parameter_struct oc;
    timer_parameter_struct init;
    
    //timer_internal_clock_config(TIMER2);
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
    
    rcu_periph_clock_enable(RCU_TIMER2);
    timer_deinit(TIMER2);

    init.prescaler         = 1;
    init.period            = para->period;
    
    init.alignedmode       = TIMER_COUNTER_EDGE;
    init.counterdirection  = TIMER_COUNTER_UP;
    init.clockdivision     = TIMER_CKDIV_DIV1;
    init.repetitioncounter = 0;
    timer_init(TIMER2,&init);
    timer_auto_reload_shadow_enable(TIMER2);
    
    oc.ocpolarity   = TIMER_OC_POLARITY_HIGH;
    oc.outputstate  = TIMER_CCX_ENABLE;
    oc.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    oc.outputnstate = TIMER_CCXN_DISABLE;
    oc.ocidlestate  = TIMER_OC_IDLE_STATE_LOW;
    oc.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    
    timer_channel_output_config(TIMER2,TIMER_CH_1, &oc);
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_1, para->pulse);
    timer_channel_output_mode_config(TIMER2, TIMER_CH_1,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER2, TIMER_CH_1,TIMER_OC_SHADOW_DISABLE);

    timer_channel_output_config(TIMER2,TIMER_CH_0, &oc);
    timer_channel_output_pulse_value_config(TIMER2, TIMER_CH_0, para->pulse);
    timer_channel_output_mode_config(TIMER2, TIMER_CH_0,TIMER_OC_MODE_PWM0);
    timer_channel_output_shadow_config(TIMER2, TIMER_CH_0,TIMER_OC_SHADOW_DISABLE);

    timer_primary_output_config(TIMER2, ENABLE);
    timer_dma_enable(TIMER2, TIMER_DMA_CH1D);
    
    return 0;
}


///////////////////////////////////////////////////////////////////////////



int dal_pwm_init(U32 freq)
{
    int r;
    pwm_tmr_para_t para;
    
    r = cal_tmr_para(freq, &para);
    if(r) {
        return -1;
    }
    
    rcu_config();
    gpio_config();
    dma_config();
    timer_config(&para);
    
    //dal_pwm_enable(1);
    
    return 0;
}


int dal_pwm_set(U32 freq)
{
    int r;
    pwm_tmr_para_t para;
    
    r = cal_tmr_para(freq, &para);
    if(r) {
        return -1;
    }
    
    timer_disable(TIMER2);
    timer_autoreload_value_config(TIMER2, para.period);
    timer_channel_output_pulse_value_config(TIMER2,TIMER_CH_1, para.pulse);
    timer_enable(TIMER2);
    
    return 0;
}


int dal_pwm_enable(U8 on)
{
    if(on) {
        timer_enable(TIMER2);
    }
    else {
        timer_disable(TIMER2);
    }
    
    return 0;
}



int dal_pwm_deinit(void)
{
    return 0;
}


