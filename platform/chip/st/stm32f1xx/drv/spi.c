#include <stdio.h>
#include "drv/spi.h"
#include "drv/dma.h"
#include "lock.h"

#if defined(__CC_ARM)
#pragma diag_suppress 1296
#endif


typedef struct {
    SPI_HandleTypeDef hspi;
    DMA_HandleTypeDef rx; 
}spi_handle_t;


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
        init.Speed = GPIO_SPEED_FREQ_HIGH;
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


handle_t spi_init(spi_cfg_t *cfg)
{
    spi_handle_t *h=calloc(1, sizeof(spi_handle_t));
    
    if(!h) {
        return NULL;
    }
    
    h->hspi.Instance = spiDef[cfg->port];
    h->hspi.Init = cfg->init;
    HAL_SPI_Init(&h->hspi);

    return h;
}


int spi_deinit(handle_t *h)
{
    spi_handle_t **sh=(spi_handle_t**)h;
    
    if(!sh || !(*sh)) {
        return -1;
    }
    
    HAL_SPI_DeInit(&(*sh)->hspi);
    free(*sh);
    
    return 0;
}


int spi_read(handle_t h, U8 *data, U16 len, U32 timeout)
{
    spi_handle_t *sh=(spi_handle_t*)h;
    
    if(!sh || !data || !len) {
        return -1;
    }
    
    return (int)HAL_SPI_Receive(&sh->hspi, data, len, timeout);//HAL_MAX_DELAY);
    
}


int spi_write(handle_t h, U8 *data, U16 len, U32 timeout)
{
    spi_handle_t *sh=(spi_handle_t*)h;
    
    if(!sh || !data || !len) {
        return -1;
    }
    
    return (int)HAL_SPI_Transmit(&sh->hspi, data, len, timeout);//HAL_MAX_DELAY);
}


int spi_readwrite(handle_t h, U8 *wdata, U8 *rdata, U16 len, U32 timeout)
{
    spi_handle_t *sh=(spi_handle_t*)h;
    
    if(!sh || !wdata || !rdata || !len) {
        return -1;
    }
    
    return (int)HAL_SPI_TransmitReceive(&sh->hspi, wdata, rdata, len, timeout);//HAL_MAX_DELAY);
}

