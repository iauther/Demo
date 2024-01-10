#include "gd32f4xx.h"
#include "protocol.h"
#include "dal.h"
#include "log.h"
#include "cfg.h"


#define STORAGE_INFO_REG       0x1FFF7A20
#define UNIQUE_ID_REG          0x1FFF7A10     //0x1FFF7A10~0x1FFF7A18


////////////////////////////////////////
__STATIC_INLINE void irq_en(U8 on)
{
    if(on) {
        __enable_irq();
        __enable_fiq();
    }
    else {
        __disable_irq();
        __disable_fiq();
    }
}



static int clk_init(void)
{
    irq_en(1);
    
#ifndef OS_KERNEL
    if(SysTick_Config(SystemCoreClock/1000)) {
        while(1);
    }
#endif
    
	return 0;
}
static int clk_deinit(void)
{
#ifndef OS_KERNEL
    SysTick->VAL = 0;
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
#endif
    
	return 0;
}



void dal_set_priority(void)
{
    //nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    //NVIC_SetPriority(SysTick_IRQn, 0x00U);
}


int dal_init(void)
{
    clk_init();
    return 0;
}


int dal_deinit(void)
{
    clk_deinit();
    return 0;
}



void dal_reboot(void)
{
    irq_en(0);
    LOGD("____ reboot now ...\n");
    NVIC_SystemReset();
}


U32 dal_get_freq(void)
{
    return SystemCoreClock;
}


int dal_get_info(mcu_info_t *info)
{
    U8 i;
    U32 v;
    
    v = REG32(STORAGE_INFO_REG);
	info->flashSize  = (v>>16)*1024;
    info->flashStart = 0;
    info->flashEnd   = info->flashStart+info->flashSize;
    
    info->sramSize   = (v&0xffff)*1024;
    info->sramStart  = 0;
    info->sramEnd    = info->sramStart+info->sramSize;
	for(i=0; i<3; i++) {
		info->uniqueID[i] = REG32(UNIQUE_ID_REG+i);
	}
    
    return 0;
}


U32 dal_get_chipid(void)
{
    return REG32(UNIQUE_ID_REG);
}


//////////////////////////////////////////////////////
#define VALUE_AT(x) (*((__IO U32*)((x)|FLASH_BASE)))
typedef void (*jump_fn_t)(void);
void dal_jump(U32 addr)
{
    U32 sp = VALUE_AT(addr);
    jump_fn_t jump_fn = (jump_fn_t)VALUE_AT(addr+4);
    
    //LOGD("___dal_jump, addr: 0x%x, sp: 0x%x, jump to:0x%x\n", addr, sp, jump_fn);
    if(sp<0x200C0000) {
        irq_en(0);
        nvic_vector_table_set(NVIC_VECTTAB_FLASH, addr);
        __set_CONTROL(0); __set_MSP(sp);
        
        jump_fn();
    }
}


void EXTI0_IRQHandler()
{
}
#if 0
void WWDGT_IRQHandler(){}
void LVD_IRQHandler(){}
void TAMPER_STAMP_IRQHandler(){}
    
void FMC_IRQHandler(){}
void RCU_CTC_IRQHandler(){}
void EXTI0_IRQHandler(){}
void EXTI1_IRQHandler(){}
void EXTI2_IRQHandler(){}
void EXTI3_IRQHandler(){}
void EXTI4_IRQHandler(){}
void DMA0_Channel0_IRQHandler(){}
void DMA0_Channel1_IRQHandler(){}
void DMA0_Channel2_IRQHandler(){}
void DMA0_Channel3_IRQHandler(){}
void DMA0_Channel4_IRQHandler(){}
void DMA0_Channel5_IRQHandler(){}

void ADC_IRQHandler(){}
void CAN0_TX_IRQHandler(){}
void CAN0_RX0_IRQHandler(){}
void CAN0_RX1_IRQHandler(){}
void CAN0_EWMC_IRQHandler(){}
void EXTI5_9_IRQHandler(){}
void TIMER0_BRK_TIMER8_IRQHandler(){}
void TIMER0_UP_TIMER9_IRQHandler(){}
void TIMER0_TRG_CMT_TIMER10_IRQHandler(){}
void TIMER0_Channel_IRQHandler(){}

    
void I2C0_EV_IRQHandler(){}
void I2C0_ER_IRQHandler(){}
void I2C1_EV_IRQHandler(){}
void I2C1_ER_IRQHandler(){}
void SPI0_IRQHandler(){}
void SPI1_IRQHandler(){}

    
void EXTI10_15_IRQHandler(){}

void USBFS_WKUP_IRQHandler(){}
void TIMER7_BRK_TIMER11_IRQHandler(){}
void TIMER7_UP_TIMER12_IRQHandler(){}
void TIMER7_TRG_CMT_TIMER13_IRQHandler(){}
void TIMER7_Channel_IRQHandler(){}
void DMA0_Channel7_IRQHandler(){}
void EXMC_IRQHandler(){}

void SPI2_IRQHandler(){}

void TIMER5_DAC_IRQHandler(){}

void DMA1_Channel2_IRQHandler(){}
void DMA1_Channel3_IRQHandler(){}
void DMA1_Channel4_IRQHandler(){}
void ENET_IRQHandler(){}
void ENET_WKUP_IRQHandler(){}
void CAN1_TX_IRQHandler(){}
void CAN1_RX0_IRQHandler(){}
void CAN1_RX1_IRQHandler(){}
void CAN1_EWMC_IRQHandler(){}
void USBFS_IRQHandler(){}
void DMA1_Channel5_IRQHandler(){}

void DMA1_Channel7_IRQHandler(){}

void I2C2_EV_IRQHandler(){}
void I2C2_ER_IRQHandler(){}
void USBHS_EP1_Out_IRQHandler(){}
void USBHS_EP1_In_IRQHandler(){}
void USBHS_WKUP_IRQHandler(){}
void USBHS_IRQHandler(){}
void DCI_IRQHandler(){}
void TRNG_IRQHandler(){}
void FPU_IRQHandler(){}

void SPI3_IRQHandler(){}
void SPI4_IRQHandler(){}
void SPI5_IRQHandler(){}
void TLI_IRQHandler(){}
void TLI_ER_IRQHandler(){}
void IPA_IRQHandler(){}


//void USART5_IRQHandler(){}
//void DMA1_Channel6_IRQHandler(){}
//void UART6_IRQHandler(){}
//void UART7_IRQHandler(){}
//void TIMER6_IRQHandler(){}
//void DMA1_Channel0_IRQHandler(){}
//void DMA1_Channel1_IRQHandler(){}
//void SDIO_IRQHandler(){}
//void TIMER4_IRQHandler(){}
//void UART3_IRQHandler(){}
//void UART4_IRQHandler(){}
//void TIMER1_IRQHandler(){}
//void TIMER2_IRQHandler(){}
//void TIMER3_IRQHandler(){}
//void USART0_IRQHandler(){}
//void USART1_IRQHandler(){}
//void USART2_IRQHandler(){}
//void RTC_Alarm_IRQHandler(){}
//void RTC_WKUP_IRQHandler(){}
//void DMA0_Channel6_IRQHandler(){}
#endif





