#include "drv/gpio.h"
#include "drv/delay.h"
#include "wdog.h"
#include "myCfg.h"

//看门狗芯片：ADM8613Y232ACBZ
//喂狗最大间隔为1.6秒, 喂狗脉冲持续时间须大于85ns

int wdog_init(void)
{
    gpio_pin_t pin=GPIO_WDG_PIN;
    
    gpio_init(&pin, MODE_OUTPUT);
    gpio_set_hl(&pin, 0);
    
    return 0;
}


int wdog_feed(void)
{
    gpio_pin_t pin=GPIO_WDG_PIN;
    
    gpio_set_hl(&pin, 1);
    delay_us(1);
    gpio_set_hl(&pin, 0);
    
    return 0;
}


int wdog_enable(U8 on)
{
    return 0;
}

