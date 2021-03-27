#include "log.h"
#include "drv/gpio.h"
#include "drv/delay.h"
#include "drv/timer.h"
#include "sim89xx.h"
#include "cfg.h"

#define SIM89XX_EN_PIN		    HAL_GPIO_15
#define SIM89XX_PWRKEY_PIN		HAL_GPIO_8
#define SIM89XX_RESET_PIN	    HAL_GPIO_9

static handle_t sim89xx_tmr_handle;
static void sim89xx_tmr_callback(void *user_data)
{
    gpio_set_hl(SIM89XX_PWRKEY_PIN, HAL_GPIO_DATA_HIGH);
    //LOGI("____ SIM89XX POWERKEY UP NOW!\r\n");
    
    timer_sw_stop(&sim89xx_tmr_handle);
}



void sim89xx_power(U8 on)
{
    timer_set_t set;
    
    if(on) {
        LOGI("____ SIM89XX POWER ENABLE\r\n");
        gpio_init(SIM89XX_EN_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_HIGH);
        gpio_pull_up(SIM89XX_EN_PIN);
        
        gpio_init(SIM89XX_RESET_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_HIGH);
        gpio_pull_up(SIM89XX_RESET_PIN);     
        
        LOGI("____ SIM89XX POWERKEY DOWN FOR 3 SECONDS!\r\n");
        gpio_init(SIM89XX_PWRKEY_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_LOW);
        
        set.ms = 3000;
        set.repeat = 0;
        set.callback = sim89xx_tmr_callback;
        set.user_data = NULL;
        sim89xx_tmr_handle = timer_sw_start(&set);
    }
    else {
        sim89xx_tmr_callback(NULL);
        gpio_pull_down(SIM89XX_RESET_PIN);
        gpio_pull_down(SIM89XX_EN_PIN);
        
        LOGI("____ SIM89XX POWER DISABLE\r\n");
        gpio_init(SIM89XX_RESET_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_LOW);
        gpio_init(SIM89XX_EN_PIN, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_LOW);
    }
}


void sim89xx_reset(void)
{
    gpio_set_hl(SIM89XX_RESET_PIN, HAL_GPIO_DATA_LOW);
    delay_ms(100);
    gpio_set_hl(SIM89XX_RESET_PIN, HAL_GPIO_DATA_HIGH);
}






