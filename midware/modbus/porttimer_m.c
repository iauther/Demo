/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
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
 * File: $Id: porttimer_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "mbdef.h"
#include "platform.h"
#include <stdio.h>

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
TIM_HandleTypeDef htim4;
static void prvvTIMERExpiredISR( void );

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortTimersInit( USHORT usTim1Timerout50us )
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 3599;								// 50us记一次数
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = usTim1Timerout50us - 1;					// usTim1Timerout50us * 50即为定时器溢出时间
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
    {
        return FALSE;
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
    {
        return FALSE;
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
    {
        return FALSE;
    }

	__HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);              // 先清除一下定时器的中断标记,防止使能中断后直接触发中断
    __HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);					// 使能定时器更新中断
    return TRUE;
}

inline void vMBMasterPortTimersEnable(  )
{
    __HAL_TIM_SET_COUNTER(&htim4, 0);		// 清空计数器
    __HAL_TIM_ENABLE(&htim4);				// 使能定时器
}

inline void vMBMasterPortTimersDisable(  )
{
    __HAL_TIM_DISABLE(&htim4);				// 禁能定时器
}

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */
static void prvvTIMERExpiredISR( void )
{
    ( void )pxMBPortCBTimerExpired(  );
}

/// 定时器4中断服务程序
void TIM4_IRQHandler(void)
{
    if(__HAL_TIM_GET_FLAG(&htim4, TIM_FLAG_UPDATE))			// 更新中断标记被置位
    {
        __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);		// 清除中断标记
        prvvTIMERExpiredISR();								// 通知modbus3.5个字符等待时间到
    }
}

#endif

