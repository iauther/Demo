#include <stdio.h>
#include "spi.h"
#include "config.h"


SPI_HandleTypeDef hspi[SPI_MAX];
SPI_TypeDef  *spiDef[SPI_MAX]={SPI1, SPI2, SPI3};
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
	GPIO_InitTypeDef init;
	
    switch((u32)hspi->Instance) {
        case (u32)SPI1:
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        init.Pin = GPIO_PIN_5|GPIO_PIN_7;
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &init);

        init.Pin = GPIO_PIN_6;
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &init);
        break;

        case (u32)SPI2:
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
        
        case (u32)SPI3:
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
    switch((u32)hspi->Instance) {

        case (u32)SPI1:
        __HAL_RCC_SPI1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
        break;

        case (u32)SPI2:
        __HAL_RCC_SPI2_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
        break;

        case (u32)SPI3:
        __HAL_RCC_SPI3_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5);
        break;
    }
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


int spi_read(eSPI spi, u8 *data, u16 len)
{
    return (int)HAL_SPI_Receive(&hspi[spi], data, len, 0xFFFF);
}


int spi_write(eSPI spi, u8 *data, u16 len)
{
    return (int)HAL_SPI_Transmit(&hspi[spi], data, len, 0xFFFF);
}


