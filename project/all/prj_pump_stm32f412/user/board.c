#include "board.h"
#include "drv/clk.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "drv/uart.h"
#include "drv/i2c.h"
#include "drv/clk.h"


U8   data_flag=0;
U32  sys_freq = 0;
handle_t i2cHandle;

////////////////////////////////////////////

static void bus_init(void)
{
    i2c_cfg_t  ic;
    
    ic.pin.scl.grp = GPIOA;
    ic.pin.scl.pin = GPIO_PIN_8;
    
    ic.pin.sda.grp = GPIOC;
    ic.pin.sda.pin = GPIO_PIN_9;
    
    ic.freq = 100000;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    i2cHandle = i2c_init(&ic);
}



void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

int board_init(void)
{
    HAL_Init();
    clk2_init();
    sys_freq = clk2_get_freq();
    
    bus_init();
   
    return 0;
}



















