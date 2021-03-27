#include "dev.h"
#include "drv/i2c.h"
#include "drv/timer.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "bio/bio.h"
#include "cfg.h"
#include "bmi160/bmi160.h"
#include "bio/ads8866.h"


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



