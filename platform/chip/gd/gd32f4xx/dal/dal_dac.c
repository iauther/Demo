#include "dal_dac.h"
#include "gd32f4xx_dac.h"
#include "cfg.h"
#include "log.h"


typedef struct {
	dal_dac_cfg_t   cfg;
}dal_dac_handle_t;

#define PRESCALER   2

#define DMA_INT_FLAGS    DMA_CHXCTL_FTFIE
//#define DMA_INT_FLAGS    (DMA_CHXCTL_SDEIE|DMA_CHXCTL_TAEIE|DMA_CHXCTL_HTFIE|DMA_CHXCTL_FTFIE)



void DMA0_Channel6_IRQHandler(void)
{
    if(dma_interrupt_flag_get(DMA0, DMA_CH6, DMA_INT_FLAG_FTF)){
        dma_interrupt_flag_clear(DMA0, DMA_CH6, DMA_INT_FLAG_FTF);
    }
}

static void gpio_config(U8 port)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    
    if(port==DAC_0) {
        gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);
    }
    else {
        gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_5);
    }
}

static void dma_config(U8 port, void *buf, int len)
{
    U8 IRQn;
    U32 dax,dma,paddr;
    dma_channel_enum chn;
    rcu_periph_enum  rcu;
    dma_subperipheral_enum sub;
    dma_single_data_parameter_struct init;
    dma_multi_data_parameter_struct  init2;

    if(port==DAC_0) {
        dax = DAC0,
        dma = DMA0;
        rcu = RCU_DMA0;
        chn = DMA_CH5;
        sub = DMA_SUBPERI7;
        IRQn = DMA0_Channel5_IRQn;
        paddr = (U32)&DAC0_L12DH;
    }
    else {
        dax = DAC1;
        dma = DMA0;
        rcu = RCU_DMA0;
        chn = DMA_CH6;
        sub = DMA_SUBPERI7;
        IRQn = DMA0_Channel6_IRQn;
        paddr = (U32)&DAC1_L12DH;
    }
    
    rcu_periph_clock_enable(rcu);
    
    init.periph_addr         = paddr;
    init.memory0_addr        = (U32)buf;
    init.direction           = DMA_MEMORY_TO_PERIPH;
    init.number              = len/2;
    init.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    init.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    init.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    init.priority            = DMA_PRIORITY_HIGH;
    init.circular_mode       = DMA_CIRCULAR_MODE_ENABLE;
    
    init2.periph_addr = paddr;
    init2.periph_width = DMA_PERIPH_WIDTH_16BIT;
    init2.memory_width = DMA_MEMORY_WIDTH_16BIT;
    init2.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    init2.memory0_addr = (U32)buf;
    init2.number = len/2;
    init2.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    init2.memory_burst_width = DMA_MEMORY_BURST_4_BEAT;
    init2.periph_burst_width = DMA_MEMORY_BURST_4_BEAT;//DMA_PERIPH_BURST_SINGLE;
    init2.critical_value = DMA_FIFO_2_WORD;
    //init2.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    init2.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    init2.direction = DMA_MEMORY_TO_PERIPH;
    init2.priority = DMA_PRIORITY_HIGH;
    
    //dma_single_data_mode_init(dma, chn, &init);
    dma_multi_data_mode_init(dma, chn, &init2);
    
    dma_channel_subperipheral_select(dma, chn, sub);
    dma_interrupt_enable(dma, chn, DMA_INT_FLAGS);
    //nvic_irq_enable(IRQn, 3, 0);
    dma_channel_enable(dma, chn);
}
static void dma_cnt_reset(dal_dac_handle_t *h)
{
    U32 dax,dma,paddr;
    dma_channel_enum chn;
    
    if(h->cfg.port==DAC_0) {
        dax = DAC0,
        dma = DMA0;
        chn = DMA_CH5;
    }
    else {
        dax = DAC1;
        dma = DMA0;
        chn = DMA_CH6;
    }
    
    dma_channel_disable(dma, chn);
    dma_transfer_number_config(dma, chn, h->cfg.bufLen/2);
    dma_channel_enable(dma, chn);
}

typedef struct {
    U32     value;
}dac_tmr_para_t;
static int cal_tmr_para(U32 freq, dac_tmr_para_t *para)
{
    U32 freqMax=2500*KHZ;
    U32 pclk=SystemCoreClock/PRESCALER;
    
    if(freq>freqMax) {
        LOGE("freq %d is overflow, the freqMax is %d\n", freq, freqMax);
        return -1;
    }
    
    if(para) para->value = pclk/freq;
    
    return 0;
}

static int tmr_config(U8 port, U32 freq)
{
    int r;
    timer_parameter_struct init;
    dac_tmr_para_t para; 
    U32 dax = (port==DAC_0)?DAC0:DAC1;

    r = cal_tmr_para(freq, &para);
    if(r) {
        return r;
    }
    
    dac_trigger_source_config(dax, DAC_TRIGGER_T7_TRGO);
    dac_trigger_enable(dax);
    
    rcu_periph_clock_enable(RCU_TIMER7);
    timer_deinit(TIMER7);
    
    timer_prescaler_config(TIMER7, PRESCALER-1, TIMER_PSC_RELOAD_UPDATE);
    timer_autoreload_value_config(TIMER7, para.value);
    timer_master_output_trigger_source_select(TIMER7, TIMER_TRI_OUT_SRC_UPDATE);
    timer_enable(TIMER7);
    
    return 0;
}
static void dac_config(U8 port)
{
    U32 dax=(port==DAC_0)?DAC0:DAC1;
    
    rcu_periph_clock_enable(RCU_DAC);
    dac_deinit();
    dac_trigger_source_config(dax, DAC_TRIGGER_T7_TRGO);
    dac_trigger_enable(dax);
    dac_wave_mode_config(dax, DAC_WAVE_DISABLE);
    dac_output_buffer_enable(dax);
    dac_enable(dax);
    dac_dma_enable(dax);
}



handle_t dal_dac_init(dal_dac_cfg_t *cfg)
{
    dal_dac_handle_t *h=calloc(1, sizeof(dal_dac_handle_t));
    if(!h) {
        return NULL;
    }
    h->cfg = *cfg;
    
    gpio_config(h->cfg.port);
    dma_config(h->cfg.port, h->cfg.buf, h->cfg.bufLen);
    tmr_config(h->cfg.port, h->cfg.freq);
    dac_config(h->cfg.port);
    
    return h;
}


int dal_dac_deinit(handle_t h)
{
    dal_dac_handle_t *dh=(dal_dac_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    free(dh);
    
    return 0;
}



int dal_dac_set_freq(handle_t h, U32 freq)
{
    int r;
    dac_tmr_para_t para;
    dal_dac_handle_t *dh=(dal_dac_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    r = cal_tmr_para(freq, &para);
    if(r) {
        return -1;
    }
    timer_autoreload_value_config(TIMER7, para.value);
    
    return 0;
}


int dal_dac_start(handle_t h)
{
    U32 dax;
    dal_dac_handle_t *dh=(dal_dac_handle_t*)h;

    if(!dh) {
        return -1;
    }
    
    dax = (dh->cfg.port==DAC_0)?DAC0:DAC1;
    dac_enable(dax); 
    
    return 0;
}


int dal_dac_stop(handle_t h)
{
    U32 dax;
    dal_dac_handle_t *dh=(dal_dac_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    //dma_cnt_reset(dh);
    dax = (dh->cfg.port==DAC_0)?DAC0:DAC1;
    dac_disable(dax);
    
    return 0;
}






