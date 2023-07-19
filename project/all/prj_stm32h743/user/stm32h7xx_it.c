#include "stm32h7xx.h"

void NMI_Handler(void)
{
  while (1)
  {
  }
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
void SVC_Handler(void)
{
}
void PendSV_Handler(void)
{
}
void SysTick_Handler(void)
{
    HAL_IncTick();
}
#endif


