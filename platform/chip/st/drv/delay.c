#include <math.h>
#include "task.h"
#include "drv/delay.h"


extern U32 sys_freq;   //the delay function depend on this freq


#define RETURN

void delay_ns(U32 ns)
{
    int m;
    
    m = ceil(1000000000.0/sys_freq);
    if(ns>0 && ns<m) {
        __nop();
    }
    else {
        int n=ceil(ns%m);
        while(n-->0) __nop();
    }
}


void delay_us(U32 us)
{   
    while(us-->0) delay_ns(1000);
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


void delay_sec(U32 sec)
{
    while(sec-->0) delay_ms(1000);
}
























