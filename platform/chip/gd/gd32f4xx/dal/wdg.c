#include "drv/wdg.h"


static IWDG_HandleTypeDef hiwg;
int wdg_init(U32 ms)
{   
    hiwg.Instance = IWDG;				        // IWDG 地址
	hiwg.Init.Prescaler = IWDG_PRESCALER_32;    //选择32分频，大概为1kHz
	hiwg.Init.Reload = ms;				    //2000<0xfff=4095,2000*1ms=2s
	
	return HAL_IWDG_Init(&hiwg);
}


int wdg_feed(void)
{
    return HAL_IWDG_Refresh(&hiwg);
}











