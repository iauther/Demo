/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "drv/uart.h"


void NMI_Handler(void)
{
}


void HardFault_Handler(void)
{
  while (1)
  {
  }
}


void MemManage_Handler(void)
{
  while (1)
  {
  }
}


void BusFault_Handler(void)
{
  while (1)
  {
  }
}


void UsageFault_Handler(void)
{
  while (1)
  {
  }
}


void DebugMon_Handler(void)
{
}

#ifndef OS_KERNEL
void PendSV_Handler(void){}
void SVC_Handler(void){}
void SysTick_Handler(void){HAL_IncTick();}
#endif

static uint32_t gpioPin[16]={GPIO_PIN_0,GPIO_PIN_1,GPIO_PIN_2,GPIO_PIN_3,GPIO_PIN_4,GPIO_PIN_5,GPIO_PIN_6,GPIO_PIN_7,
                             GPIO_PIN_8,GPIO_PIN_9,GPIO_PIN_10,GPIO_PIN_11,GPIO_PIN_12,GPIO_PIN_13,GPIO_PIN_14,GPIO_PIN_15};
static void gpio_irq_handle(void)
{
    int i;
    for(i=0;i<16;i++) {
        HAL_GPIO_EXTI_IRQHandler(gpioPin[i]);
    }
}

void EXTI0_IRQHandler(void)
{
    gpio_irq_handle();
}
void EXTI1_IRQHandler(void)
{
    gpio_irq_handle();
}
void EXTI2_IRQHandler(void)
{
    gpio_irq_handle();
}
void EXTI3_IRQHandler(void)
{
    gpio_irq_handle();
}
void EXTI4_IRQHandler(void)
{
    gpio_irq_handle();
}
void EXTI5_IRQHandler(void)
{
    gpio_irq_handle();
}

extern void uart_handler(int uart);
extern void uart_dma_handler(char rxtx, int uart);
void USART1_IRQHandler(void)
{
    uart_handler(UART_1);
}
void USART2_IRQHandler(void)
{
    uart_handler(UART_2);
}
void USART3_IRQHandler(void)
{
    uart_handler(UART_3);
}

void DMA2_Stream2_IRQHandler(void)
{
    uart_dma_handler(0, UART_1);    //rx
}
void DMA2_Stream5_IRQHandler(void)
{
    uart_dma_handler(0, UART_1);    //rx
}
void DMA2_Stream7_IRQHandler(void)
{
    uart_dma_handler(1, UART_1);    //tx
}

void DMA1_Stream5_IRQHandler(void)
{
    uart_dma_handler(0, UART_2);    //rx
}
void DMA1_Stream7_IRQHandler(void)
{
    uart_dma_handler(0, UART_2);    //rx
}
void DMA1_Stream6_IRQHandler(void)
{
    uart_dma_handler(1, UART_2);    //tx
}

void DMA1_Stream1_IRQHandler(void)
{
    uart_dma_handler(0, UART_3);    //rx
}
void DMA1_Stream3_IRQHandler(void)
{
    uart_dma_handler(1, UART_3);    //tx
}
void DMA1_Stream4_IRQHandler(void)
{
    uart_dma_handler(1, UART_3);    //tx
}


void TIM6_IRQHandler(void)
{
    extern void htimer_irq_handler();
    htimer_irq_handler();
}


