#include "power.h"
#include "myCfg.h"
#include "drv/gpio.h"
#include "drv/delay.h"
 

int power_init(void)
{
    int i;
    gpio_pin_t p1=GPIO_PAD_PWR_PIN;
    gpio_pin_t p2=GPIO_PAD_PWR2_PIN;
    
    gpio_init(&p1, MODE_OUTPUT);
    gpio_init(&p2, MODE_OUTPUT);
    power_on(PDEV_PAD, 1);

#if 0    
    while(1) {
        power_on(PDEV_PAD, 0);
        delay_ms(5);
        power_on(PDEV_PAD, 1);
        delay_ms(5);
    }
#endif
    
    return 0;
}


int power_on(U8 pdev, U8 on)
{
    if(pdev>=PDEV_MAX) {
        return -1;
    }
    
    if(pdev==PDEV_PAD) {
        U8 hl=on?1:0;
        gpio_pin_t p1=GPIO_PAD_PWR_PIN;
        gpio_pin_t p2=GPIO_PAD_PWR2_PIN;
        
        gpio_set_hl(&p1, hl);
        gpio_set_hl(&p2, hl);
    }
        
    return 0;
}


