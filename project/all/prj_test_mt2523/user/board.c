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
#include "temp/temp.h"
#include "myCfg.h"

handle_t i2c0Handle=NULL;
handle_t i2c1Handle=NULL;
handle_t i2c2Handle=NULL;

////////////////////////////////////////////

static int bus_init(void)
{
    i2c_cfg_t  ic;
    i2c_pin_t  p0={I2C0_SCL_PIN, I2C0_SDA_PIN};
    i2c_pin_t  p2={I2C2_SCL_PIN, I2C2_SDA_PIN};
    
    ic.pin= p0;
    ic.freq = I2C0_FREQ;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    i2c0Handle = i2c_init(&ic);
    
    ic.pin= p2;
    i2c2Handle = i2c_init(&ic);
    
    return 0;
}

static int bus_deinit(void)
{
    int r;
    
    r = i2c_deinit(&i2c0Handle);
    r = i2c_deinit(&i2c2Handle);
    
    return r;
}



static int dev_init(void)
{
    int r=0;

    //r = nvm_init();
    r = temp_init(i2c2Handle);
    r = led_init();    
    
    return r;
}
static int dev_deinit(void)
{
    int r=0;
    
    return r;
}



int board_init(void)
{
    int r;
    U8  color;
    
    r = clk2_init();
    
    r = bus_init();
    r = dev_init();
    
    //r = paras_load();
   
    return r;
}


int board_deinit(void)
{
    int r;
   
    return r;
}
















