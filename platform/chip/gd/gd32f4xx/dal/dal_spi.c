#include "dal_spi.h"
#include "dal_pwm.h"
#include "dal_dac.h"
#include "dal_gpio.h"
#include "gd32f4xx_spi.h"
#include "dal_delay.h"
#include "log.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif


#define DMA_INT_FLAGS    (DMA_CHXCTL_HTFIE|DMA_CHXCTL_FTFIE)
//#define DMA_INT_FLAGS    (DMA_CHXCTL_FTFIE)


typedef struct {
	dal_spi_cfg_t   cfg;
}dal_spi_handle_t;


typedef struct {
	U32   dma;
    U32   dmaChn;
    U32   dmaRcu;
}spi_dma_info_t;

typedef struct {
	U8   bits;
   
    
}spi_pin_info_t;


static dal_gpio_t spiPIN[SPI_MAX] = {0};
static U32        spiDMA[SPI_MAX] = {0};
static U32        spiPORT[SPI_MAX]={SPI0,SPI1,SPI2,SPI3,SPI4,SPI5};
static dal_spi_handle_t* spiHandle[SPI_MAX]={NULL};

static void spi_dma_init(dal_spi_handle_t *h)
{
    U32 port=spiPORT[h->cfg.port];
    dma_single_data_parameter_struct init;
    dma_multi_data_parameter_struct  init2;
    
    rcu_periph_clock_enable(RCU_DMA1);
    
    dma_deinit(DMA1, DMA_CH6);
    dma_single_data_para_struct_init(&init);
    
    //set dma rx
    init.direction = DMA_PERIPH_TO_MEMORY;
    init.memory0_addr = (U32)h->cfg.buf.rx.buf;
    init.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    init.periph_memory_width = DMA_MEMORY_WIDTH_16BIT;
    init.number = h->cfg.buf.rx.blen/2;
    init.periph_addr = (U32)&SPI_DATA(port);
    init.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    init.priority = DMA_PRIORITY_HIGH;
    //init.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    init.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    
    init2.periph_addr = (U32)&SPI_DATA(SPI5);;
    init2.periph_width = DMA_PERIPH_WIDTH_16BIT;
    init2.memory_width = DMA_MEMORY_WIDTH_16BIT;
    init2.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    init2.memory0_addr = (U32)h->cfg.buf.rx.buf;
    init2.number = h->cfg.buf.rx.blen/2;
    init2.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    init2.memory_burst_width = DMA_MEMORY_BURST_4_BEAT;
    init2.periph_burst_width = DMA_MEMORY_BURST_4_BEAT;//DMA_PERIPH_BURST_SINGLE;
    init2.critical_value = DMA_FIFO_4_WORD;
    //init2.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    init2.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    init2.direction = DMA_PERIPH_TO_MEMORY;
    init2.priority = DMA_PRIORITY_ULTRA_HIGH;
    
    dma_multi_data_mode_init(DMA1, DMA_CH6, &init2);
    //dma_single_data_mode_init(DMA1, DMA_CH6, &init);
    
    dma_channel_subperipheral_select(DMA1, DMA_CH6, DMA_SUBPERI1);
    dma_flow_controller_config(DMA1, DMA_CH6, DMA_FLOW_CONTROLLER_DMA);
    
    dma_interrupt_enable(DMA1, DMA_CH6, DMA_INT_FLAGS);
    nvic_irq_enable(DMA1_Channel6_IRQn, 4, 0);
    

#if 0
    //set dma tx
    dma_deinit(DMA1, DMA_CH5);
    
    init.direction = DMA_MEMORY_TO_PERIPH;
    init.memory0_addr = (U32)h->cfg.tx.buf;
    init.number = h->cfg.tx.blen/2;
    init.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    
    dma_single_data_mode_init(DMA1, DMA_CH5, &init);
    dma_channel_subperipheral_select(DMA1, DMA_CH5, DMA_SUBPERI1);
    dma_flow_controller_config(DMA1, DMA_CH5, DMA_FLOW_CONTROLLER_DMA);
    dma_interrupt_enable(DMA1, DMA_CH5, DMA_INT_FLAGS);
    
    nvic_irq_enable(DMA1_Channel5_IRQn, 3, 0);
#endif
}

volatile void* p_dma=NULL;
void DMA1_Channel6_IRQHandler(void)     //spi dma rx
{
    U32 cnt;
    //U32 v1,v2,v3,t1,t2,t3,flag=0;
    dal_spi_handle_t *h=spiHandle[SPI_5];
    
    //dma_channel_disable(DMA1, DMA_CH6);
    //dal_pwm_stop();
    
    //t1=dal_get_tick();
    
    cnt = h->cfg.buf.rx.blen/4;     //双buffer，取总个数的一半
    if(dma_interrupt_flag_get(DMA1, DMA_CH6, DMA_INT_FLAG_HTF)) {
        dma_interrupt_flag_clear(DMA1, DMA_CH6, DMA_INT_FLAG_HTF);
 p_dma = (U16*)h->cfg.buf.rx.buf;
        if(h && h->cfg.callback) {
            h->cfg.callback((U16*)h->cfg.buf.rx.buf, cnt);
        }
    }
    else if(dma_interrupt_flag_get(DMA1, DMA_CH6, DMA_INT_FLAG_FTF)) {
        dma_interrupt_flag_clear(DMA1, DMA_CH6, DMA_INT_FLAG_FTF);
p_dma = (U16*)h->cfg.buf.rx.buf+cnt;        
        if(h && h->cfg.callback) {
            h->cfg.callback((U16*)h->cfg.buf.rx.buf+cnt, cnt);
        }
    }
    //t3=dal_get_tick();
    
    //LOGD("%d, %d ms\n", t2-t1, t3-t2);
    
    //dma_memory_address_config(DMA1, DMA_CH6, DMA_MEMORY_0, (U32)h->cfg.rx.buf);
    //dma_transfer_number_config(DMA1, DMA_CH6, h->cfg.rx.blen/2);
    
    //dal_pwm_start();
    //dma_channel_enable(DMA1, DMA_CH6);
}


//spi 0/3/4/5接APB2,最大频率120MHz
//spi 1/2接APB1,    最大频率60MHz
handle_t dal_spi_init(dal_spi_cfg_t *cfg)
{
    U32 port,pin;
    spi_parameter_struct init; 
    dal_spi_handle_t *h=NULL;
    U32 mode[4]={SPI_CK_PL_LOW_PH_1EDGE,SPI_CK_PL_LOW_PH_2EDGE,SPI_CK_PL_HIGH_PH_1EDGE,SPI_CK_PL_HIGH_PH_2EDGE};
    
    if(!cfg) {
        return NULL;
    }
    
    h = calloc(1, sizeof(dal_spi_handle_t));
    if(!h) {
        return NULL;
    }
    
    h->cfg = *cfg;
    
    spiHandle[h->cfg.port] = h;
    
    port = spiPORT[h->cfg.port];
    
    rcu_periph_clock_enable(RCU_GPIOG); // 使用G端口
    rcu_periph_clock_enable(RCU_SPI5);  // 使能SPI5
    
    
#if 0
    pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14;
    
    if(!h->cfg.softCs) {
        pin |= GPIO_PIN_8;
    }
    
    gpio_af_set(GPIOG, GPIO_AF_5, pin);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, pin);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, pin);    
#else
    
    #if 1
    pin = GPIO_PIN_13;
    if(!h->cfg.softCs) {
        pin |= GPIO_PIN_8;
    }
    
    gpio_af_set(GPIOG, GPIO_AF_5, pin);
    gpio_mode_set(GPIOG, GPIO_MODE_AF, GPIO_PUPD_NONE, pin);
    gpio_output_options_set(GPIOG, GPIO_OTYPE_PP, GPIO_OSPEED_MAX, pin);
    #endif
    
    pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;//|GPIO_PIN_14;
    gpio_af_set(GPIOG, GPIO_AF_5, pin);
    gpio_mode_set(GPIOG, GPIO_MODE_INPUT, GPIO_PUPD_NONE, pin);
#endif    

    if(h->cfg.useDMA) {
        spi_dma_init(h);
    }

    spi_i2s_deinit(SPI5);
    spi_struct_para_init(&init);
    
    /* configure SPIx parameter */
    init.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    init.device_mode = h->cfg.mast?SPI_MASTER:SPI_SLAVE;
    init.frame_size = (h->cfg.bits==8)?SPI_FRAMESIZE_8BIT:SPI_FRAMESIZE_16BIT;
    init.clock_polarity_phase = mode[cfg->mode];
    init.nss = (h->cfg.softCs)?SPI_NSS_SOFT:SPI_NSS_HARD;
    init.prescale = SPI_PSC_2;//SPI_PSC_4;
    init.endian = (h->cfg.isMsb)?SPI_ENDIAN_MSB:SPI_ENDIAN_LSB;
    
    spi_init(port, &init);
    if(h->cfg.quad) {
        qspi_enable(port);
    }
    
    if(!h->cfg.softCs) {
        //spi_nss_output_disable(port);      //拉高cs
        //spi_ti_mode_enable(port);
    }
    spi_enable(port);
    
    if(h->cfg.useDMA) {
        dma_channel_enable(DMA1, DMA_CH6);
        //dma_channel_enable(DMA1, DMA_CH5);        //disable spi tx dma
        
        spi_dma_enable(port, SPI_DMA_RECEIVE);
        spi_dma_enable(port, SPI_DMA_TRANSMIT);
    }
    
    return h;
}


int dal_spi_deinit(handle_t h)
{
    U32 port;
    dal_spi_handle_t *dh=(dal_spi_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    port = spiPORT[dh->cfg.port];
    spi_disable(port);
    spi_i2s_deinit(port);
    free(dh);
    spiHandle[port] = NULL;
    
    return 0;
}


static inline void spi_r(U32 port, U16 *dat)
{
    while (RESET == spi_i2s_flag_get(port, SPI_FLAG_RBNE));
    *dat = spi_i2s_data_receive(port);
}
static inline void spi_w(U32 port, U16 dat)
{   
    while (RESET == spi_i2s_flag_get(port, SPI_FLAG_TBE));
    spi_i2s_data_transmit(port, dat); 
}



int dal_spi_read(handle_t h, U16 *data, U16 cnt, U32 timeout)
{
    int i;
    U32 port;
    dal_spi_handle_t *dh=(dal_spi_handle_t*)h;
    
    if(!dh || !data || !cnt) {
        return -1;
    }
    
    port = spiPORT[dh->cfg.port];
    for(i=0; i<cnt; i++) {
        spi_r(port, data+i);
    }
    
    return 0;
}


int dal_spi_write(handle_t h, U16 *data, U16 cnt, U32 timeout)
{
    int i;
    U32 port;
    dal_spi_handle_t *dh=(dal_spi_handle_t*)h;
    
    if(!dh || !data || !cnt) {
        return -1;
    }
    
    port = spiPORT[dh->cfg.port];
    for(i=0; i<cnt; i++) {
        spi_w(port, data[i]);
    }
    
    return 0;
}


static U16 spi_rw_byte(U32 port, U16 data)
{
    /* wait data register emplty */
    while(RESET == spi_i2s_flag_get(port, SPI_FLAG_TBE));
    
    /* send byte through the port */
    spi_i2s_data_transmit(port, data);
    
    /* wait to receive a byte */
    while(RESET == spi_i2s_flag_get(port, SPI_FLAG_RBNE));
    while(SET == spi_i2s_flag_get(port, SPI_STAT_TRANS));
    
    return spi_i2s_data_receive(port);
}

int dal_spi_readwrite(handle_t h, U16 *wdata, U16 *rdata, U16 cnt, U32 timeout)
{
    int i;
    U32 port;
    dal_spi_handle_t *dh=(dal_spi_handle_t*)h;
    
    if(!dh || !wdata || !rdata || !cnt) {
        return -1;
    }
    
    port = spiPORT[dh->cfg.port];
    for(i=0; i<cnt; i++) {
        rdata[i] = spi_rw_byte(port, wdata[i]);
    }
    
    return 0;
}


int dal_spi_set_hw_cs(handle_t h, U8 hl)
{
    U32 port;
    dal_spi_handle_t *dh=(dal_spi_handle_t*)h;
    
    if(!dh) {
        return -1;
    }
    
    port = spiPORT[dh->cfg.port];
    if(hl) {
        spi_nss_output_disable(port);
    }
    else {
        spi_nss_output_enable(port);
    }
    
    return 0;
}


int dal_spi_enable(U8 on)
{
    if(on) {
        spi_enable(SPI5);
    }
    else {
        spi_disable(SPI5);
    }
    
    return 0;
}
