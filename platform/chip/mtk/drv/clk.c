#include "clk.h"
#include "platform.h"


int clk2_init(void)
{
    hal_clock_init();
    SystemCoreClockUpdate();
    
    return 0;
}


U32 clk2_get_freq(void)
{
    return hal_clock_get_mcu_clock_frequency();
}











