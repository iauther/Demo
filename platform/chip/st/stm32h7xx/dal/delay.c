#include "dal/delay.h"


extern U32 sys_freq;   //the delay function depend on this freq


static void inline hw_delay_us(U32 us)
{
    U32 tickStart, tickCur, tickCnt;
    U32 tickMax = SysTick->LOAD;
    U32 udelay_value = (SysTick->LOAD/1000)*us;

    tickStart = SysTick->VAL;
    while(1) {
        tickCur = SysTick->VAL;
        tickCnt = (tickStart < tickCur) ? (tickMax+tickStart-tickCur) : (tickStart-tickCur);
        if (tickCnt > udelay_value)
            break;
    }
}



static void inline sw_delay_us(U32 us)
{
    int x,y;
    
    while(us-->0) {
        x=ceil(1000000000.0/sys_freq);
        y=1000/x;
        
        while(y-->0) __nop();
    }
}


void delay_us(U32 us)
{   
    hw_delay_us(us);
    //sw_delay_us(us);
}


void delay_ms(U32 ms)
{
#ifdef OS_KERNEL
    if(osKernelGetState()==osKernelRunning) {
        osDelay(ms);
    }
    else {
        HAL_Delay(ms);
    }
#else
    HAL_Delay(ms);
#endif
    
}   


void delay_s(U32 s)
{
    while(s-->0) delay_ms(1000);
}


#ifdef OS_KERNEL
#include "platform.h"

uint32_t HAL_GetTick (void) {
    return ((uint32_t)osKernelGetTickCount ());
}

#endif



















