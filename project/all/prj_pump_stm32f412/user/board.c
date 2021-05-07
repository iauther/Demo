#include "board.h"
#include "data.h"
#include "drv/clk.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "drv/uart.h"
#include "drv/i2c.h"
#include "drv/clk.h"
#include "paras.h"
#include "cfg.h"

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


static void dev_init(void)
{
    //ft32xx_init();
    
    paras_load();

#ifdef OS_KERNEL
    //n950_init();
    ms4525_init();
    bmp280_init();
    valve_init();
#endif
}
static void dev_deinit(void)
{
#ifdef OS_KERNEL
    //n950_deinit();
    ms4525_deinit();
    bmp280_deinit();
    valve_deinit();
#endif
}




void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

int board_init(void)
{
    SCB->VTOR = FLASH_BASE | APP_OFFSET;
    __enable_irq();
    
    HAL_Init();
    clk2_init();
    sys_freq = clk2_get_freq();
    
    bus_init();
    dev_init();
   
    return 0;
}


int board_deinit(void)
{
    HAL_DeInit();
    HAL_RCC_DeInit();
    i2c_deinit(&i2cHandle);
    
    dev_deinit();
   
    return 0;
}
















