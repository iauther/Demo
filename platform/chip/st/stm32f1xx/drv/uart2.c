/**
  ******************************************************************************
  * File Name          : USART.c
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "config.h"


UartParas_t uartParas[UART_MAX]={0};
UART_HandleTypeDef uartHandle[UART_MAX];
DMA_HandleTypeDef  uartHdma[UART_MAX];

DMA_Channel_TypeDef      *dmaChannel[UART_MAX]={DMA1_Channel5, DMA1_Channel6, DMA1_Channel3, DMA2_Channel3, NULL};
const USART_TypeDef      *uartDef[UART_MAX]={USART1, USART2, USART3, UART4, UART5};
const IRQn_Type          uartIRQn[UART_MAX]={USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn};
const IRQn_Type          dmaIRQn[UART_MAX]={DMA1_Channel5_IRQn, DMA1_Channel6_IRQn, DMA1_Channel3_IRQn, DMA2_Channel3_IRQn, NonMaskableInt_IRQn};



static eUART get_uart(UART_HandleTypeDef *huart)
{
    eUART uart;

    switch((u32)huart->Instance) {
        case (u32)USART1:
        {
            uart = UART_1;
        }
        break;

        case (u32)USART2:
        {
            uart = UART_2;
        }
        break;

        case (u32)USART3:
        {
            uart = UART_3;
        }
        break;

        case (u32)UART4:
        {
            uart = UART_4;
        }
        break;

        case (u32)UART5:
        default:
        {
            uart = UART_5;
        }
        break;
    }
    return uart;
}


static int uart_irq_set(eUART uart, int on)
{
    IRQn_Type IRQn = uartIRQn[uart];
    
    if(on) {
        if(IRQn==DMA2_Channel3_IRQn) {
            __HAL_RCC_DMA2_CLK_ENABLE();
        }
        else if(IRQn!=NonMaskableInt_IRQn) {
            __HAL_RCC_DMA1_CLK_ENABLE();
        }

        HAL_NVIC_SetPriority(IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(IRQn);
    }
    else {
        HAL_NVIC_DisableIRQ(IRQn);
    }
    
    return 0;
}

static int hdma_init(eUART uart)
{
    DMA_HandleTypeDef  *hdma=&uartHdma[uart];
    
    if(uart<UART_5) {
        hdma->Instance = dmaChannel[uart];
        hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma->Init.PeriphInc = DMA_PINC_DISABLE;
        hdma->Init.MemInc = DMA_MINC_ENABLE;
        hdma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma->Init.Mode = DMA_CIRCULAR;
        hdma->Init.Priority = DMA_PRIORITY_HIGH;
        if (HAL_DMA_Init(hdma) != HAL_OK) {
            return -1;
        }

        __HAL_LINKDMA(&uartHandle[uart], hdmarx, *hdma);
        HAL_NVIC_SetPriority(dmaIRQn[uart], 1, 0);
        HAL_NVIC_EnableIRQ(dmaIRQn[uart]);
    }
    uart_irq_set(uart, 1);

    return 0;
}


void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef init = {0};

    switch((u32)huart->Instance) {
        case (u32)USART1:
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        init.Pin = GPIO_PIN_9;
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &init);

        init.Pin = GPIO_PIN_10;
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &init);
        break;
        
        case (u32)USART2:
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        init.Pin = GPIO_PIN_2;
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &init);

        init.Pin = GPIO_PIN_3;
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &init);
        break;
        
        case (u32)USART3:
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        init.Pin = GPIO_PIN_10;
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &init);

        init.Pin = GPIO_PIN_11;
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &init);
        break;
        
        case (u32)UART4:
        __HAL_RCC_UART4_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        init.Pin = GPIO_PIN_10;
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOC, &init);

        init.Pin = GPIO_PIN_11;
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &init);
        break;
        
        case (u32)UART5:
        __HAL_RCC_UART5_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();

        init.Pin = GPIO_PIN_12;
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOC, &init);

        init.Pin = GPIO_PIN_2;
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOD, &init);
        break;
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    switch((u32)huart->Instance) {
        case (u32)USART1:
        __HAL_RCC_USART1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
        break;
        
        case (u32)USART2:
        __HAL_RCC_USART2_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
        break;
        
        case (u32)USART3:
        __HAL_RCC_USART3_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);
        break;
        
        case (u32)UART4:
        __HAL_RCC_UART4_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11);
        break;
        
        case (u32)UART5:
        __HAL_RCC_UART5_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_12);
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
        break;
    }
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    eUART uart = get_uart(huart);
    
    switch(uartParas[uart].mode) {
        case MODE_IT:
        uartParas[uart].rx.dlen++;
        HAL_UART_Receive_IT(huart, uartParas[uart].rx.buf+uartParas[uart].rx.dlen, 1);
        break;
        
        case MODE_DMA:
        //HAL_UART_Receive_DMA(huart, uartParas[uart].rx.buf, uartParas[uart].rx.blen);
        break;
        
        default:
        break;
    }
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    eUART uart = get_uart(huart);
    
    if(uartParas[uart].tx.fn) {
        uartParas[uart].tx.fn(huart);
    }
    
    switch(uartParas[uart].mode) {
        case MODE_IT:
        uartParas[uart].tx.dlen++;
        HAL_UART_Transmit_IT(huart, uartParas[uart].tx.buf+uartParas[uart].tx.dlen, 1);
        break;

        case MODE_DMA:
        HAL_UART_Transmit_DMA(huart, uartParas[uart].tx.buf, uartParas[uart].tx.blen);
        break;

        default:
        break;
    }
}


void HAL_UART_IdleCallback(UART_HandleTypeDef *huart)
{
    u16 datalen;
    u32 st = 0;
    eUART uart = get_uart(huart);
    
    __HAL_UART_CLEAR_IDLEFLAG(huart);

    //st = huart->Instance->SR;
    //st = huart->Instance->DR;

    //HAL_NVIC_DisableIRQ(dmaIRQn[uart]);
    //__HAL_DMA_DISABLE(huart->hdmarx);
    //HAL_UART_DMAStop(huart);
    
    datalen = uartParas[uart].rx.blen - __HAL_DMA_GET_COUNTER(huart->hdmarx);
    if(uartParas[uart].idl) {
        uartParas[uart].idl(huart, datalen);
    }
    uartParas[uart].rx.dlen = 0;

    //HAL_NVIC_EnableIRQ(dmaIRQn[uart]);
    //__HAL_DMA_ENABLE(huart->hdmarx);
    HAL_UART_Receive_DMA(huart, uartParas[uart].rx.buf, uartParas[uart].rx.blen);
}


int usart_init(eUART uart, UART_InitTypeDef *init)
{
    UART_HandleTypeDef *huart=&uartHandle[uart];

    huart->Instance = (USART_TypeDef*)uartDef[uart];
    huart->Init = *init;
    return (int)HAL_UART_Init(huart);
}


int usart_deinit(eUART uart)
{
    return (int)HAL_UART_DeInit(&uartHandle[uart]);
}


int usart_dma_en(eUART uart, UartParas_t *para)
{
    hdma_init(uart);

    uartParas[uart] = *para;
    uartParas[uart].mode = MODE_DMA;
    __HAL_UART_ENABLE_IT(&uartHandle[uart], UART_IT_IDLE);
    HAL_UART_Receive_DMA(&uartHandle[uart], uartParas[uart].rx.buf, uartParas[uart].rx.blen);
    HAL_UART_Transmit_DMA(&uartHandle[uart], uartParas[uart].tx.buf, uartParas[uart].tx.blen);
    
    return 0;
}


int usart_it_en(eUART uart, UartParas_t *para)
{
    uartParas[uart] = *para;
    uartParas[uart].mode = MODE_IT;
    __HAL_UART_ENABLE_IT(&uartHandle[uart], UART_IT_IDLE); 
    HAL_UART_Receive_IT(&uartHandle[uart], uartParas[uart].rx.buf, 1);
    HAL_UART_Transmit_IT(&uartHandle[uart], uartParas[uart].tx.buf, 1);

    return 0;
}


int usart_read(eUART uart, u8 *data, u16 len)
{
    return (int)HAL_UART_Receive(&uartHandle[uart], data, len, 0xFFFF);
}


int usart_write(eUART uart, u8 *data, u16 len)
{
    return (int)HAL_UART_Transmit(&uartHandle[uart], data, len, 0xFFFF);
}

