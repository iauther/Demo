#include "dal_pmu.h"
#include "gd32f4xx_pmu.h"
#include "dal_gpio.h"
#include "log.h"




int dal_pmu_init(void)
{
    rcu_periph_clock_enable(RCU_PMU);
    
    pmu_backup_write_enable();
    pmu_wakeup_pin_enable();
    
    return 0;
}





int dal_pmu_set(U8 mode)
{
    //U8 cmd=WFI_CMD;
    U8 cmd=WFE_CMD;
    
    switch(mode) {
        case PMU_SLEEP:
        {
            pmu_to_sleepmode(cmd);
        }
        break;
        
        case PMU_DEEPSLEEP:
        {
            pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, cmd);
        }
        break;
        
        case PMU_STANDBY:
        {
            pmu_to_standbymode(cmd);
        }
        break;
        
        default:
        return -1;
        
    }
    
    return 0;
}







