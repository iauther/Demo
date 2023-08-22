#include "dal_delay.h"

/*
        SysTick寄存器：
        CTRL：SysTick控制及状态寄存器
        LOAD：SysTick重装载数值寄存器
        VAL：SysTick当前数值寄存器
        CALIB：SysTick校准数值寄存器
*/


volatile static U32 systick_ms = 0;
volatile static U64 systick_us = 0;
volatile static F32 lsb_us = (1.0F/30);


U64 dal_get_tick_us(void)
{
    U64 tick_us;
    
    tick_us = SysTick->VAL*lsb_us;
    
    return (systick_us+tick_us);
}


U32 dal_get_tick_ms(void)
{
    return systick_ms;
}


static void delay_us(U32 us)
{
    U64 end, cur=dal_get_tick_us();
    
    if(us==0) {
        return;
    }
    
    end = cur + us;
    while(dal_get_tick_us()<end);
}
static void delay_ms(U32 ms)
{
    U32 end, cur=dal_get_tick_ms();
    
    if(ms==0) {
        return;
    }
    
    end = cur + ms;    
    while(dal_get_tick_ms()<end);
}



void systick_handler(void)
{
    systick_us += 1000;
    systick_ms += 1;
}



void dal_delay_us(U32 us)
{   
    delay_us(us);
}


void dal_delay_ms(U32 ms)
{
#ifdef OS_KERNEL
    if(osKernelGetState()==osKernelRunning) {
        osDelay(ms);
    }
    else {
        delay_ms(ms);
    }
#else
    delay_ms(ms);
#endif
    
}   


void dal_delay_s(U32 s)
{
    while(s-->0) dal_delay_ms(1000);
}


U32 dal_get_tick (void)
{
#ifdef OS_KERNEL
    return ((U32)osKernelGetTickCount());
#else
    return systick_ms;
#endif
}



