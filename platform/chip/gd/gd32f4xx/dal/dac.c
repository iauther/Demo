#include "dal_dac.h"
#include "gd32f4xx_dac.h"
#include "cfg.h"


#define BSP_TIMER_RCU  				RCU_TIMER5
#define BSP_TIMER      				TIMER5
#define BSP_TIMER_IRQ  				TIMER5_DAC_IRQn



typedef struct 
	
}dac_handle_t;


static void dac_set_freq(uint16_t hz)
{
    if(hz>max_wav_fps) hz=max_wav_fps;
    
    if(hz>1000) 
    {
        TIMER_PSC(TIMER6) = 0;
        TIMER_CAR(TIMER6) = 200000000 / 100 / hz -1;
    }
    else if(hz>10) 
    {
        TIMER_PSC(TIMER6) = 10-1;
        TIMER_CAR(TIMER6) = 200000000 / 1000 / hz -1;
    }
    else
    {
        TIMER_PSC(TIMER6) = 50-1;
        TIMER_CAR(TIMER6) = 200000000 / 5000 / hz -1;
    }
}



int dac_init(void)
{
    dac_handle_t *h=NULL;
    
    h=calloc(1, sizeof(dac_handle_t));
    if(!h) {
        return -1;
    }
    
    
    
    adcHandle = h;
    
    return 0;
}


int dac_config(dac_cfg_t *cfg)
{
    dma_single_data_parameter_struct dma_struct;
    
    //GPIO使能
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_5);
    
    //DAC使能
    rcu_periph_clock_enable(RCU_DAC);
    dac_deinit();
    dac_trigger_source_config(DAC1, DAC_TRIGGER_T6_TRGO);
    dac_trigger_enable(DAC1);
    dac_wave_mode_config(DAC1, DAC_WAVE_DISABLE);
    dac_output_buffer_enable(DAC1);
    /* enable DAC1 and DMA for DAC1 */
    dac_enable(DAC1);
    dac_dma_enable(DAC1);
    
    //DMA使能
    rcu_periph_clock_enable(RCU_DMA0);
    dma_channel_subperipheral_select(DMA0, DMA_CH6, DMA_SUBPERI7);
    dma_struct.periph_addr         = (uint32_t)&DAC1_R8DH;
    dma_struct.memory0_addr        = (uint32_t)convertarr;
    dma_struct.direction           = DMA_MEMORY_TO_PERIPH;
    dma_struct.number              = 100;
    dma_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_struct.priority            = DMA_PRIORITY_ULTRA_HIGH;
    dma_struct.circular_mode       = DMA_CIRCULAR_MODE_ENABLE;
    dma_single_data_mode_init(DMA0, DMA_CH6, &dma_struct);
    dma_channel_enable(DMA0, DMA_CH6);
    
    //定时器使能
    rcu_periph_clock_enable(RCU_TIMER6);
    timer_prescaler_config(TIMER6, 9, TIMER_PSC_RELOAD_UPDATE);
    timer_autoreload_value_config(TIMER6, 0xff);
    timer_master_output_trigger_source_select(TIMER6, TIMER_TRI_OUT_SRC_UPDATE);
    timer_enable(TIMER6);
    
    //dac_set_freq(1000); 
    return 0;
}


int dac_deinit(void)
{
    dachandle_t *h=dacHandle;
    
    
    free(h);
    
    return 0;
}



int dac_start(void)
{
    dac_handle_t *h=dacHandle;
    

    return 0;
}

int adc_stop(void)
{
    ac_handle_t *h=dacHandle;
    
   
    
    return 0;
}


/////////////////////////////////////////////////////////////////
#define BSP_TIMER_RCU  				RCU_TIMER5
#define BSP_TIMER      				TIMER5
#define BSP_TIMER_IRQ  				TIMER5_DAC_IRQn
#define BSP_TIMER_IRQHANDLER  TIMER5_DAC_IRQHandler
void basic_timer_config(uint16_t pre,uint16_t per)
{
	timer_parameter_struct timere_initpara;
	
	rcu_periph_clock_enable(BSP_TIMER_RCU);
	rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
	
	
	timer_deinit(BSP_TIMER);
	
	timere_initpara.prescaler = pre-1;                       
	timere_initpara.alignedmode = TIMER_COUNTER_EDGE;                      
	timere_initpara.counterdirection = TIMER_COUNTER_UP;                
	timere_initpara.clockdivision = TIMER_CKDIV_DIV1;                  
	timere_initpara.period = per-1;                          
	timere_initpara.repetitioncounter = 0; 
	
	timer_init(BSP_TIMER,&timere_initpara);
	
	
	nvic_irq_enable(BSP_TIMER_IRQ,3,2);
	
	timer_interrupt_enable(BSP_TIMER,TIMER_INT_UP);
	
	timer_enable(BSP_TIMER);
}

void capture_timer_config(void)
{
    /* TIMER2 configuration: input capture mode -------------------
    the external signal is connected to TIMER2 CH0 pin (PB4)
    the rising edge is used as active edge
    the TIMER2 CH0CV is used to compute the frequency value
    ------------------------------------------------------------ */
    timer_ic_parameter_struct timer_icinitpara;
    timer_parameter_struct timer_initpara;
        //开启定时器时钟

	rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_TIMER2);
    gpio_mode_set(GPIOA,GPIO_MODE_INPUT,GPIO_PUPD_NONE,GPIO_PIN_6);
	 
	
    timer_deinit(TIMER2);

    /* TIMER2 configuration */
    timer_initpara.prescaler         = 200-1;//定时器的时钟频率是120MHz,预分频120-1
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;//对齐模式
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;//向上计数
    timer_initpara.period            = 65535;//重载值
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;//不分频
    timer_initpara.repetitioncounter = 0;//重复计数
    timer_init(TIMER2,&timer_initpara);
//      timer_channel_input_struct_para_init(&timer_icinitpara);
    /* TIMER2  configuration */
    /* TIMER2 CH0 input capture configuration */
    timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;//捕获极性,上升沿捕获
    timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;//通道输入模式选择
    timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;//分频
    timer_icinitpara.icfilter    = 0x0;//滤波
    timer_input_capture_config(TIMER2,TIMER_CH_0,&timer_icinitpara);
     nvic_irq_enable(TIMER2_IRQn,4,2);
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER2);//自动重载使能
    /* clear channel 0 interrupt bit */
    timer_interrupt_flag_clear(TIMER2,TIMER_INT_FLAG_CH0);//CH0 通道中断清除
    /* channel 0 interrupt enable */
    timer_interrupt_enable(TIMER2,TIMER_INT_CH0);//CH0 通道中断使能	

    /* TIMER2 counter enable */
    timer_enable(TIMER2);
}




void TIMER5_DAC_IRQHandler(void)
{
  if(timer_interrupt_flag_get(BSP_TIMER,TIMER_INT_FLAG_UP) == SET)
	{
		timer_interrupt_flag_clear(BSP_TIMER,TIMER_INT_FLAG_UP);
		/* 执行功能 */
		//gpio_bit_toggle(PORT_LED2,PIN_LED2);
		//printf("BSP_TIMER_IRQHANDLER!\r\n");
	}
}


#endif
