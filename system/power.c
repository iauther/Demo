#include "power.h"
#include "ecxxx.h"
#include "ads9120.h"
#include "dal_pmu.h"
#include "dal_gpio.h"
#include "dal_rtc.h"
#include "log.h"
#include "rtc.h"
#include "cfg.h"

typedef struct {
    handle_t h4g;
}pwr_handle_t;
static pwr_handle_t pwrHandle;
    

int power_init(void)
{
    rtcx_init();
    //dal_pmu_init();
    
    return 0;
}


#ifndef BOOTLOADER
int power_set_dev(U8 dev, U8 mode)
{
    switch (dev) {
        
        case POWER_DEV_PMU:
        {
            switch(mode) {
                case POWER_MODE_SLEEP:
                dal_pmu_set(PMU_SLEEP);
                break;
                
                case POWER_MODE_DEEPSLEEP:
                dal_pmu_set(PMU_DEEPSLEEP);
                break;
                
                case POWER_MODE_STANDBY:
                dal_pmu_set(PMU_STANDBY);
                break;
                
                default:
                return -1;
            }
        }
        break;
        
        case POWER_DEV_ECXXX:
        {
            switch(mode) {
                case POWER_MODE_ON:
                break;
                
                case POWER_MODE_OFF:
                break;
                
                case POWER_MODE_SLEEP:
                break;
                
                case POWER_MODE_DEEPSLEEP:
                break;
                
                case POWER_MODE_STANDBY:
                break;
                
                default:
                return -1;
            }
        }
        break;
        
        case POWER_DEV_ADS9120:
        {
            switch(mode) {
                case POWER_MODE_ON:
                ads9120_power(1);
                break;
                
                case POWER_MODE_OFF:
                ads9120_power(0);
                break;
                
                default:
                return -1;
            }
        }
        break;
        
        default:
        return -1;
    }
    
    return 0;
}


int power_set(U8 mode)
{
    
    switch(mode) {
        case POWER_MODE_ON:
        {
            power_set_dev(POWER_DEV_ADS9120, 1);
            //power_set_dev(POWER_DEV_ECXXX,   1);
        }
        break;
        
        case POWER_MODE_OFF:
        {
            
        }
        break;
        
        case POWER_MODE_SLEEP:
        {
            power_set_dev(POWER_DEV_ADS9120, POWER_MODE_OFF);
            //power_set_dev(POWER_DEV_ECXXX, POWER_MODE_SLEEP);
            power_set_dev(POWER_DEV_PMU, POWER_MODE_SLEEP);
        }
        break;
        
        case POWER_MODE_DEEPSLEEP:
        {
            power_set_dev(POWER_DEV_ADS9120, POWER_MODE_OFF);
            power_set_dev(POWER_DEV_PMU, POWER_MODE_DEEPSLEEP);
            //power_set_dev(DEV_ADS9120, MODE_ON);
        }
        break;
        
        case POWER_MODE_STANDBY:
        {
            power_set_dev(POWER_DEV_ADS9120, POWER_MODE_OFF);
            power_set_dev(POWER_DEV_PMU, POWER_MODE_STANDBY);
            //power_set_dev(POWER_DEV_ADS9120, POWER_MODE_ON);
        }
        break;
        
        default:
        return -1;
    }

    return 0;
}
#endif


int power_off(U32 wakeup_ms)
{
    int r,i;
    
    for(i=0; i<10; i++) {
        r = rtcx_set_countdown(wakeup_ms);
        if(r==0) {
            return 0;
        }
    }
    
    return -1;
}


