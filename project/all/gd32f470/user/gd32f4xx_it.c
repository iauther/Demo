/*!
    \file    gd32f4xx_it.c
    \brief   interrupt service routines

    \version 2016-08-15, V1.0.0, firmware for GD32F4xx
    \version 2018-12-12, V2.0.0, firmware for GD32F4xx
    \version 2020-09-30, V2.1.0, firmware for GD32F4xx
    \version 2022-03-09, V3.0.0, firmware for GD32F4xx
*/

/*
    Copyright (c) 2022, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "gd32f4xx_it.h"
#include "log.h"

/*!
    \brief    this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
}

/*!
    \brief    this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    LOGE("--- HardFault\n");
    
#if 0
    #define NVIC_CTRL                  REG32(0xE000ED24)
    #define NVIC_MFSR                  REG32(0xE000ED28)
    #define NVIC_BFSR                  REG32(0xE000ED29)
    #define NVIC_UFSR                  REG32(0xE000ED2A)
    #define NVIC_HFSR                  REG32(0xE000ED2C)
    #define NVIC_DFSR                  REG32(0xE000ED30)
    #define NVIC_MMAR                  REG32(0xE000ED34)
    #define NVIC_BFAR                  REG32(0xE000ED38)
    #define NVIC_AFSR                  REG32(0xE000ED3C)
                       
    if (NVIC_HFSR & (1u << 31)) {
        NVIC_HFSR |=  (1u << 31);     // Reset Hard Fault status
        *(pStack + 6u) += 2u;         // PC is located on stack at SP + 24 bytes. Increment PC by 2 to skip break instruction.
        return;                       // Return to interrupted application
    }
    
    LOGE("--- CTRL: 0x%08x\n", NVIC_CTRL);
    LOGE("--- MFSR: 0x%08x\n", NVIC_MFSR);
    LOGE("--- BFSR: 0x%08x\n", NVIC_BFSR);
    LOGE("--- UFSR: 0x%08x\n", NVIC_UFSR);
    LOGE("--- HFSR: 0x%08x\n", NVIC_HFSR);
    LOGE("--- HFSR: 0x%08x\n", NVIC_HFSR);
    LOGE("--- DFSR: 0x%08x\n", NVIC_DFSR);
    LOGE("--- MMAR: 0x%08x\n", NVIC_MMAR);
    LOGE("--- BFAR: 0x%08x\n", NVIC_BFAR);
    LOGE("--- AFSR: 0x%08x\n", NVIC_AFSR);
        
#endif
    
    while(1);
}

/*!
    \brief    this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    LOGE("--- MemoryFault\n");
    while(1) {
    }
}

/*!
    \brief    this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    LOGE("--- MemoryBusFault\n");
    while(1) {
    }
}

/*!
    \brief    this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while(1) {
    }
}


/*!
    \brief    this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
}



#ifndef OS_KERNEL
void SVC_Handler(void){}
void PendSV_Handler(void){}
void SysTick_Handler(void){
    extern void systick_handler(void);
    systick_handler();
}
#endif


