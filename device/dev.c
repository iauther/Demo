#include "dev.h"
#include "drv/i2c.h"
#include "drv/htimer.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "bio/bio.h"
#include "myCfg.h"
#include "bmi160/bmi160.h"
#include "bmp280/bmp280.h"
#include "bio/ads8866.h"


#define TMR_PORT                HAL_GPT_1
handle_t i2c0_handle;

#ifdef BOARD_V01_02
static void pwrkey_release(U8 on)   //high: release pwrkey, low: press pwrkey
{
    gpio_pin_t p={PWRCUT_PIN};
    U8 hl=on?1:0;
    
    gpio_set_hl(&p, hl);
}
#endif

static void io_init(void)
{
    gpio_pin_t p;
    
#ifdef BOARD_V00_01
    p.bin = HAL_GPIO_38;
    gpio_init(&p, MODE_INPUT);
    
    p.bin = HAL_GPIO_39;
    gpio_init(HAL_GPIO_39, MODE_INPUT);
#endif
    
#ifdef BOARD_V01_02
    p.pin = HAL_GPIO_39;
    gpio_init(&p, MODE_OUTPUT);
    
    p.pin = COMM_IN_PIN;
    //gpio_init(&p,  MODE_OUTPUT);
    
    p.pin = COMM_OUT_PIN;
    //gpio_init(&p, MODE_OUTPUT);
#endif
    
    gpio_init(&p, MODE_OUTPUT);
}




static void bio_power(U8 on)
{
    gpio_pin_t p={BIO_POWER_PIN};
    
    
    
#ifdef BOARD_V01_02
    pwrkey_release(1);          //开机后必须松开pwrkey，否则将自动关机
#endif
    
    if(on) {
        gpio_pull_up(&p);
        gpio_init(&p, MODE_OUTPUT);
        gpio_set_hl(&p, 1);
        
    }
    else {
        gpio_pull_down(&p);
        gpio_init(&p, MODE_OUTPUT);
        gpio_set_hl(&p, 0);
    }
}

static void i2c_bus_init(void)
{
    i2c_cfg_t cfg;
    
    cfg.pin.scl.pin = I2C0_SCL_PIN;
    cfg.pin.sda.pin = I2C0_SDA_PIN;
    cfg.freq = I2C0_FREQ;
    cfg.useDMA = 0;
    i2c0_handle = i2c_init(&cfg);
}



int dev_init(void)
{
    bio_handle_t *b;
    htimer_cfg_t cfg={TMR_PORT};
    
    //sim89xx_power(1);
    bio_power(1);
    
    htimer_init(&cfg);
    i2c_bus_init();
    
    bmp280_init();
    
    b = bio_probe();
    if(!b) {
        return -1;
    }
    
    gpio_ext_init();
    
    return 0;
}


int dev_check(U8 dev)
{
    int r=0;
    
    switch(dev) {
        case DEV_BMX160:
        {
            U8 status;
            r = bmi160_get_pmu_status(&status);
        }
        break;
        
        case DEV_BIOSIG:
        if(bioDevice==AD8233) {
            r = ads8866_check();
            if(r) {
                r = ads8866_reset();
            }
        }
        break;
        
        case DEV_TCA6424A:
        r = tca6424a_check();
        if(r) {
            gpio_ext_init();
        }
        break;
        
        default:
        r = -1;
        break;
    }
    
    return r;
}


int dev_reset(U8 dev)
{
    int r=0;
    
    switch(dev) {
        case DEV_BMX160:
        r = bmi160_reset();
        break;
        
        case DEV_BIOSIG:
        if(bioDevice==AD8233) {
            r = ads8866_reset();
        }
        break;
        
        case DEV_TCA6424A:
        r = gpio_ext_init();
        break;
        
        default:
        r = -1;
        break;
    }
    
    return r;
}



