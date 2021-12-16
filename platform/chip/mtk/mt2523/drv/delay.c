#include "drv/delay.h"
#include "drv/clk.h"
#include "myCfg.h"
#include "cmsis_os2.h" 

void delay_ns(int ns)
{
    U32 m,freq;

    freq = clk2_get_freq();
    m = ceil(1000000000.0/freq);
    if(ns>0 && ns<m) {
        __nop();
    }
    else {
        int n=ceil(ns%m);
        while(n-->0) __nop();
    }
}


void delay_us(int us)
{
    hal_gpt_delay_us(us);
}


void delay_ms(int ms)
{
    hal_gpt_delay_ms(ms);
}


void delay_s(int s)
{
    int i;
    for(i=0; i<s; i++) {
        delay_ms(1000);
    }
}




void os_delay_ms(int ms)
{
#ifdef OS_KERNEL
    osDelay(ms);
#endif
}











