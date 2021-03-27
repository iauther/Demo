#include "drv/gpio.h"
#include "drv/delay.h"
#include "valve.h"

//����ʹ��PA4��û�����Ϊɶ��gpioһ��ʼ����Ѵ��ںܶ�ֵ�ı�
 
#define VALVE_PIN   {GPIOC, GPIO_PIN_10}
static U8 valve_stat=0;


int valve_init(void)
{
    gpio_pin_t pin=VALVE_PIN;
    
    gpio_init(&pin, MODE_OUTPUT);
    valve_set(VALVE_CLOSE);
    
    return 0;
}



int valve_set(U8 v)
{
    gpio_pin_t pin=VALVE_PIN;
    
    gpio_set_hl(&pin, (v==VALVE_OPEN)?1:0);
    valve_stat = v;
    
    return 0;
}


int valve_get(U8 *v)
{
    if(!v) {
        return -1;
    }
    *v = valve_stat;
    
    return 0;
}






