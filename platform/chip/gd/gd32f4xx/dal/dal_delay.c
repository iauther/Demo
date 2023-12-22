#include "dal_delay.h"

/*
        SysTick寄存器：
        CTRL：SysTick控制及状态寄存器
        LOAD：SysTick重装载数值寄存器
        VAL：SysTick当前数值寄存器
        CALIB：SysTick校准数值寄存器
*/

volatile U64 timestamp_ms = 0;
volatile static U32 systick_ms = 0;
volatile static U64 systick_us = 0;
volatile static F32 lsb_us = (1.0F/30);


#define TICK_MS  (systick_ms)
#define TICK_US  (systick_us+SysTick->VAL*lsb_us)

U64 dal_get_tick_us(void)
{
    return TICK_US;
}


U32 dal_get_tick_ms(void)
{
    return TICK_MS;
}


static void delay_us(U32 us)
{
    U64 end,cur=TICK_US;
    
    if(us==0) {
        return;
    }
    
    end = cur + us;
    while(TICK_US<end);
}
static void delay_ms(U32 ms)
{
    U32 end,cur=TICK_MS;
    
    if(ms==0) {
        return;
    }
    
    end = cur + ms;
    while(TICK_MS<end);
}



void systick_handler(void)
{
    systick_us += 1000;
    systick_ms ++;
    timestamp_ms++;
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


U64 dal_get_timestamp(void)
{
    return timestamp_ms;
}


void dal_set_timestamp(U64 ts)
{
    timestamp_ms = ts;
}

