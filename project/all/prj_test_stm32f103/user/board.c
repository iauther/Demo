#include "board.h"
#include "drv/clk.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "drv/uart.h"
#include "drv/i2c.h"
#include "drv/clk.h"
#include "paras.h"
#include "power.h"
#include "wdog.h"
#include "lcd.h"
#include "drv/adc.h"
#include "task.h"
#include "myCfg.h"

handle_t i2c1Handle=NULL;

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
    lcd_cfg_t cfg={LCD_COLOR, LCD_WIDTH, LCD_HEIGHT};
    
    r = HAL_Init();
    r = clk2_init();
    
#ifdef OS_KERNEL
    task_init();
    task_start();
#endif
   
    return r;
}


int board_deinit(void)
{
    int r;
    
    r = HAL_DeInit();
    r = HAL_RCC_DeInit();
   
    return r;
}
















