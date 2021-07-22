#include "board.h"
#include "data.h"
#include "led.h"
#include "buzzer.h"
#include "data.h"
#include "drv/clk.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "drv/uart.h"
#include "drv/i2c.h"
#include "drv/clk.h"
#include "drv/htimer.h"
#include "paras.h"
#include "power.h"
#include "wdog.h"
#include "task.h"
#include "myCfg.h"
#include "temp/temp.h"

handle_t i2c0Handle=NULL;
handle_t i2c1Handle=NULL;
handle_t i2c2Handle=NULL;

////////////////////////////////////////////

static int bus_init(void)
{
    i2c_cfg_t  ic;
    i2c_pin_t  p2={I2C2_SCL_PIN,I2C2_SDA_PIN};
    
    ic.pin= p2;
    ic.freq = I2C2_FREQ;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    i2c2Handle = i2c_init(&ic);
    
    return ((i2c1Handle&&i2c2Handle)?-1:0);
}

static int bus_deinit(void)
{
    int r;
    
    r = i2c_deinit(&i2c1Handle);
    r |= i2c_deinit(&i2c2Handle);
    
    return r;
}



static int dev_init(void)
{
    int r=0;

    r = temp_init(i2c2Handle);
    
    return r;
}
static int dev_deinit(void)
{
    int r=0;
    
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
    
    r = HAL_Init();
    r = clk2_init();
    
    r = bus_init();
    r = dev_init();
    
    r = paras_load();

#ifdef OS_KERNEL
    led_set_color(GREEN);
    task_start();
#endif
   
    return r;
}


int board_deinit(void)
{
    int r;
    
    r = HAL_DeInit();
    r = HAL_RCC_DeInit();
    r = htimer_deinit();
    
    r = dev_deinit();
    r = bus_deinit();
   
    return r;
}
















