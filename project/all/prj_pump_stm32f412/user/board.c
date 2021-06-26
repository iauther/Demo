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
#include "myCfg.h"

handle_t i2c1Handle=NULL;
handle_t i2c2Handle=NULL;

////////////////////////////////////////////

static int bus_init(void)
{
    i2c_cfg_t  ic;
    i2c_pin_t  p1={I2C1_SCL_PIN,I2C1_SDA_PIN};
    i2c_pin_t  p2={I2C2_SCL_PIN,I2C2_SDA_PIN};
    
    
    ic.pin= p1;
    ic.freq = I2C1_FREQ;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    i2c1Handle = i2c_init(&ic);
    
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

    r = nvm_init();
    r = led_init();
    r = buzzer_init();
    
#ifdef OS_KERNEL
    r = n950_init();
    r = ms4525_init();
    r = bmp280_init();
    r = valve_init();
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
    r = htimer_init();
    
    r = bus_init();
    r = dev_init();

#ifdef OS_KERNEL
    led_set_color(GREEN);
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
















