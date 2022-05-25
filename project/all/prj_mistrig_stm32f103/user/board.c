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
#include "drv/spi.h"

handle_t i2c1Handle=NULL;
handle_t spi1Handle=NULL;

////////////////////////////////////////////

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
}


static void i2c_bus_init(void)
{
    i2c_cfg_t  ic;
    i2c_pin_t  p1={I2C1_SCL_PIN,I2C1_SDA_PIN};
    
    
    ic.pin= p1;
    ic.freq = I2C1_FREQ;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    i2c1Handle = i2c_init(&ic); 
}



static void bus_init(void)
{
    i2c_bus_init();
}




int board_init(void)
{
    int r;
    
    r = HAL_Init();
    r = clk2_init();
    
    bus_init();
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
















