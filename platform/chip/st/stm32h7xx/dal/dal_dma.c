#include "dal_dma.h"

//DMA_Channel_TypeDef      *dmaChannel[UART_MAX]={DMA1_Channel5, DMA1_Channel6, DMA1_Channel3, DMA2_Channel3, NULL};
//const USART_TypeDef      *uartDef[UART_MAX]={USART1, USART2, USART3, UART4, UART5};
//const IRQn_Type          uartIRQn[UART_MAX]={USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn};
//const IRQn_Type          dmaIRQn[UART_MAX]={DMA1_Channel5_IRQn, DMA1_Channel6_IRQn, DMA1_Channel3_IRQn, DMA2_Channel3_IRQn, NonMaskableInt_IRQn};

dma_para_t m_dma_para;

int dal_dma_init(DMA_HandleTypeDef *hdma)
{
    hdma->Instance = DMA2_Stream0;
    //hdma->Init.Channel = DMA_CHANNEL_3;
    hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma->Init.PeriphInc = DMA_PINC_DISABLE;
    hdma->Init.MemInc = DMA_MINC_ENABLE;
    hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma->Init.Mode = DMA_CIRCULAR;
    hdma->Init.Priority = DMA_PRIORITY_HIGH;
    hdma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(hdma) != HAL_OK) {
        return -1;
    }
    
	__HAL_RCC_DMA2_CLK_ENABLE();

    /* DMA2_Stream0_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    
    return 0;
}


int dal_dma_deinit(DMA_HandleTypeDef *hdma)
{
    HAL_DMA_DeInit(hdma);
    return 0;
}


int dal_dma_en(dma_para_t *para)
{
    m_dma_para = *para;
    //__HAL_SPI_ENABLE_IT(SPI1, SPI_IT_IDLE);
    //HAL_SPI_Receive_DMA(SPI1, m_dma_para.rx.buf, m_dma_para.rx.blen);
    //HAL_SPI_Transmit_DMA(SPI1, m_dma_para.tx.buf, m_dma_para.tx.blen);
    
    return 0;
}



