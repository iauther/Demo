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

    r = wdog_init();
    r = power_init();
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

static int tight_check(void)
{
    int cnt=150;
    ms4525_t m;
    F32 prev;
    
    ms4525_get(&m, 0);
    prev = m.pres;
    
    valve_set(VALVE_OPEN);
    while(cnt-->0) {
        
        delay_ms(1000);
        ms4525_get(&m, 0);
        if((m.pres<1.0F) && (prev-m.pres<0.01F)) {
            bias_value = m.pres;
            break;
        }
        
        prev = m.pres;
    }
    valve_set(VALVE_CLOSE);
    
    return 0;
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

#ifdef OS_KERNEL
    task_init();
#endif
    
    r = htimer_init(NULL);
    r = bus_init();
    r = dev_init();
    
    r = paras_load();
    
    //tight_check();

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
















