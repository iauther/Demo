#include "board.h"
#include "drv/clk.h"
#include "task.h"



////////////////////////////////////////////

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
}




int board_init(void)
{
    int r;
    
    r = HAL_Init();
    r = clk2_init();
    
    task_start();  
   
    return r;
}


int board_deinit(void)
{
    int r;
    
    r = HAL_DeInit();
    r = HAL_RCC_DeInit();
   
    return r;
}
















