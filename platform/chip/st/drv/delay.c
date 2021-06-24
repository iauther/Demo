#include <math.h>
#include "task.h"
#include "drv/delay.h"


extern U32 sys_freq;   //the delay function depend on this freq


static inline void delay_1us(void)
{
    int x=ceil(1000000000.0/sys_freq);
    int y=1000/x;
    
    while(y-->0) __nop();
}


void delay_us(U32 us)
{   
    while(us-->0) delay_1us();
}


void delay_ms(U32 ms)
{
#ifdef OS_KERNEL
    if(osKernelGetState()==osKernelRunning) {
        osDelay(ms);
    }
    else {
        //while(ms-->0) delay_us(1000);
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
























