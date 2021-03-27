#include "dev.h"
#include "drv/clk.h"
#include "platform.h"
#include "drv/i2c.h"
#include "drv/timer.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "cfg.h"


#define TMR_PORT                HAL_GPT_1
handle_t i2c0handle;

static void bio_power(U8 on)
{
    if(on) {
        gpio_pull_up(BIO_POWER_PIN);
        gpio_init(BIO_POWER_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_HIGH);
    }
    else {
        gpio_pull_down(BIO_POWER_PIN);
        gpio_init(BIO_POWER_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_LOW);
    }
}

static void i2c_bus_init(void)
{
    i2c_cfg_t cfg;
    
#ifdef BOARD_V00_01
    gpio_init(HAL_GPIO_38, HAL_GPIO_DIRECTION_INPUT, HAL_GPIO_DATA_LOW);
    gpio_init(HAL_GPIO_39, HAL_GPIO_DIRECTION_INPUT, HAL_GPIO_DATA_LOW);
#endif
    
    cfg.pin.scl.pin = I2C0_SCL_PIN;
    cfg.pin.sda.pin = I2C0_SDA_PIN;
    cfg.freq = I2C0_FREQ;
    cfg.useDMA = 0;
    i2c0handle = i2c_init(&cfg);
}


static int chip_init(void)
{
    clk2_init();
    
	//hal_flash_init();
	hal_nvic_init();
    __enable_irq();
	//__enable_fault_irq();
    
    return 0;
}


int board_init(void)
{
    bio_handle_t *b;
    
    chip_init();
    
    sim89xx_power(1);
    bio_power(1);
    
    timer_init(TMR_PORT);
    i2c_bus_init();
    
    b = bio_probe();
    if(!b) {
        return -1;
    }
    
    gpio_ext_init();
    
    return 0;
}

