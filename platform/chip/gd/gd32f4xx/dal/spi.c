#include "incs.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif



int spi_init()
{

    return 0;
}


static void spi_dma_init(void)
{
    dma_single_data_parameter_struct dma_init_struct;
    
    /* enable DMA1 clock */
    rcu_periph_clock_enable(RCU_DMA0);
    /* initialize DMA0 channel5 */
    dma_deinit(DMA0, DMA_CH5);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPH;
    dma_init_struct.memory0_addr = (uint32_t)0;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.periph_memory_width = DMA_MEMORY_WIDTH_8BIT; //DMA_MEMORY_WIDTH_32BIT
    dma_init_struct.number = (uint16_t)(38400);
    dma_init_struct.periph_addr = (uint32_t)&SPI_DATA(SPI2) ;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH5, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH5);
    dma_channel_subperipheral_select(DMA0, DMA_CH5, DMA_SUBPERI0);
    
//    /* enable DMA0 transfer complete interrupt */
    dma_interrupt_enable(DMA0, DMA_CH5, DMA_CHXCTL_FTFIE);
    nvic_irq_enable(DMA0_Channel5_IRQn, 0, 0);
    //dma_channel_disable(DMA0, DMA_CH5);
    
}



int spi_config(spi_cfg_t *cfg)
{
    spi_parameter_struct spi_init_struct; 

    rcu_periph_clock_enable(RCU_GPIOC); // 使用C端口
    rcu_periph_clock_enable(RCU_SPI2);  // 使能SPI2
    
    gpio_af_set(GPIOC, GPIO_AF_6, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    //gpio_bit_set(GPIOC, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);

    /* configure SPI2 parameter */
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX; // 传输模式全双工
    spi_init_struct.device_mode = SPI_MASTER;              // 配置为主机
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;       // 8位数据
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.nss = SPI_NSS_SOFT; // 软件cs
    spi_init_struct.prescale = SPI_PSC_2;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init(SPI2, &spi_init_struct);
    
    spi_dma_enable(SPI2,SPI_DMA_TRANSMIT);
    /* enable SPI */
    spi_enable(SPI2);
    
    return 0;
}


int spi_deinit(U8 spi)
{
    return 0;
}


int spi_read(U8 spi, U8 *data, U16 len, U32 timeout)
{
    while (RESET == spi_i2s_flag_get(SPI2, SPI_FLAG_RBNE));
    return spi_i2s_data_receive(SPI2);
    
    return 0;
}


int spi_write(U8 spi, U8 *data, U16 len, U32 timeout)
{
    while (RESET == spi_i2s_flag_get(SPI2, SPI_FLAG_TBE)) ;
    spi_i2s_data_transmit(SPI2, dat); 
    
    return 0;
}


static U8 spi_rw_byte(U8 data)
{
    while(RESET == spi_i2s_flag_get(SPI2, SPI_FLAG_TBE));
    spi_i2s_data_transmit(SPI2, data);

    while(RESET == spi_i2s_flag_get(SPI2, SPI_FLAG_RBNE));
    while(SET == spi_i2s_flag_get(SPI2, SPI_STAT_TRANS));
    return spi_i2s_data_receive(SPI2);
}

int spi_readwrite(U8 spi, U8 *wdata, U8 *rdata, U16 len, U32 timeout)
{
    int i;
    
    for(i=0; i<len; i++) {
        rdata[i] = spi_rw_byte(wdata[i])
    }
    
    return 0;
}

