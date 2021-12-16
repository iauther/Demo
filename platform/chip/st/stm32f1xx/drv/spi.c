#include <stdio.h>
#include "drv/spi.h"
#include "drv/dma.h"
#include "lock.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif

DMA_HandleTypeDef hdma_spi1_rx;
SPI_HandleTypeDef hspi[SPI_MAX];
SPI_TypeDef  *spiDef[SPI_MAX]={SPI1, SPI2, SPI3};
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
	GPIO_InitTypeDef init;
	
    switch((U32)hspi->Instance) {
        case (U32)SPI1:
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        init.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
        init.Mode = GPIO_MODE_AF_PP;
        init.Pull = GPIO_NOPULL;
        init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        init.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOA, &init);
        
        //__HAL_LINKDMA(hspi,hdmarx,hdma_spi1_rx);
        //dma_init(&hdma_spi1_rx);
        break;

        case (U32)SPI2:
        __HAL_RCC_SPI2_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        init.Pin = GPIO_PIN_13|GPIO_PIN_15;
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &init);

        init.Pin = GPIO_PIN_14;
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &init);
        break;
        
        case (U32)SPI3:
        __HAL_RCC_SPI3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        init.Pin = GPIO_PIN_3|GPIO_PIN_5;
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &init);

        init.Pin = GPIO_PIN_4;
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &init);
        break;
    }
}


void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    switch((U32)hspi->Instance) {

        case (U32)SPI1:
        __HAL_RCC_SPI1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
        break;

        case (U32)SPI2:
        __HAL_RCC_SPI2_CLK_DISABLE();
        //HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
        break;

        case (U32)SPI3:
        __HAL_RCC_SPI3_CLK_DISABLE();
        //HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5);
        break;
    }
    
    //dma_deinit(hspi->hdmarx);
}


int spi_init(eSPI spi, SPI_InitTypeDef *init)
{
    hspi[spi].Instance = spiDef[spi];
    hspi[spi].Init = *init;

    return (int)HAL_SPI_Init(&hspi[spi]);
}


int spi_deinit(eSPI spi)
{
    return (int)HAL_SPI_DeInit(&hspi[spi]);
}


int spi_read(eSPI spi, U8 *data, U16 len, U32 timeout)
{
    return (int)HAL_SPI_Receive(&hspi[spi], data, len, timeout);//HAL_MAX_DELAY);
}


int spi_write(eSPI spi, U8 *data, U16 len, U32 timeout)
{
    return (int)HAL_SPI_Transmit(&hspi[spi], data, len, timeout);//HAL_MAX_DELAY);
}


int spi_readwrite(eSPI spi, U8 *wdata, U8 *rdata, U16 len, U32 timeout)
{
    return (int)HAL_SPI_TransmitReceive(&hspi[spi], wdata, rdata, len, timeout);//HAL_MAX_DELAY);
}

