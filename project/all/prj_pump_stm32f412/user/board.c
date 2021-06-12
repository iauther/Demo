#include "board.h"
#include "data.h"
#include "drv/clk.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "drv/uart.h"
#include "drv/i2c.h"
#include "drv/clk.h"
#include "paras.h"
#include "myCfg.h"

handle_t i2c1Handle=NULL;
handle_t i2c2Handle=NULL;

////////////////////////////////////////////

static int bus_init(void)
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
    i2c1Handle = i2c_init(&ic);
    
    
    ic.pin.scl.grp = GPIOF;
    ic.pin.scl.pin = GPIO_PIN_1;
    ic.pin.sda.grp = GPIOF;
    ic.pin.sda.pin = GPIO_PIN_0;
    
    ic.freq = 100000;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    i2c2Handle = i2c_init(&ic);
    
    return (i2c2Handle?-1:0);
}


static int dev_init(void)
{
    int r=0;

#ifdef OS_KERNEL
    r = nvm_init();
    r = n950_init();
    r = ms4525_init();
    r = bmp280_init();
    r = valve_init();
    
    //r = mlx90632_init();
    //r = as62xx_init();
#endif
    
    return r;
}
static int dev_deinit(void)
{
    int r=0;
    
#ifdef OS_KERNEL
    r = n950_deinit();
    r = ms4525_deinit();
    r = bmp280_deinit();
    r = valve_deinit();
#endif
    
    return r;
}




void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

int board_init(void)
{
    int r;
    
#ifdef OS_KERNEL
    SCB->VTOR = FLASH_BASE | APP_OFFSET;
    __enable_irq();
#endif
    
    r = HAL_Init();
    r = clk2_init();
    
    r = bus_init();
    r = dev_init();
    r = paras_load();
   
    return r;
}


int board_deinit(void)
{
    int r;
    
    r = HAL_DeInit();
    r = HAL_RCC_DeInit();
    r = i2c_deinit(&i2c1Handle);
    r = i2c_deinit(&i2c2Handle);
    
    r = dev_deinit();
   
    return r;
}
















