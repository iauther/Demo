/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

#include "mbdef.h"
#include "platform.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

UART_HandleTypeDef huart1;
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
void vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    if(xRxEnable)
    {
		//
		// 如果使用了485控制芯片,那么在此处将485设置为接收模式
		//
		
		// MAX485_SetMode(MODE_RECV);
		
        __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);		// 使能接收非空中断
    }
    else
    {
        __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);		// 禁能接收非空中断
    }

    if(xTxEnable)
    {
		//
		// 如果使用了485控制芯片,那么在此处将485设置为发送模式
		//
		
		// MAX485_SetMode(MODE_SENT);
		
        __HAL_UART_ENABLE_IT(&huart1, UART_IT_TXE);			// 使能发送为空中断
    }
    else
    {
        __HAL_UART_DISABLE_IT(&huart1, UART_IT_TXE);		// 禁能发送为空中断
    }
}

BOOL xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = ulBaudRate;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    switch(eParity)
    {
    // 奇校验
    case MB_PAR_ODD:
        huart1.Init.Parity = UART_PARITY_ODD;
        huart1.Init.WordLength = UART_WORDLENGTH_9B;			// 带奇偶校验数据位为9bits
        break;

    // 偶校验
    case MB_PAR_EVEN:
        huart1.Init.Parity = UART_PARITY_EVEN;
        huart1.Init.WordLength = UART_WORDLENGTH_9B;			// 带奇偶校验数据位为9bits
        break;

    // 无校验
    default:
        huart1.Init.Parity = UART_PARITY_NONE;
        huart1.Init.WordLength = UART_WORDLENGTH_8B;			// 无奇偶校验数据位为8bits
        break;
    }
    return HAL_UART_Init(&huart1) == HAL_OK ? TRUE : FALSE;
}

BOOL xMBPortSerialPutByte( CHAR ucByte )
{
    USART1->TDR = ucByte;
    return TRUE;
}

BOOL xMBPortSerialGetByte( CHAR * pucByte )
{
    *pucByte = (USART1->RDR & (uint16_t)0x00FF);
    return TRUE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}


void USART1_IRQHandler(void)
{
    if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE))			// 接收非空中断标记被置位
    {
        __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);			// 清除中断标记
        prvvUARTRxISR();										// 通知modbus有数据到达
    }

    if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE))				// 发送为空中断标记被置位
    {
        __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_TXE);			// 清除中断标记
        prvvUARTTxReadyISR();									// 通知modbus数据可以发松
    }
}
