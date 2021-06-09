#include "drv/i2c.h"
#include "drv/gpio.h"
#include "drv/si2c.h"
#include "drv/delay.h"
#include "ms4525.h"
#include "cfg.h"

//≤Ó—πº∆

//#define USE_SI2C



#define MS4525_ADDR         (0x4b)

#define PMAX                (15)        //PSI
#define PMIN                (-15)       //PSI

#define PSI_OF(x)           ((x-0.05F*16383)*(PMAX-PMIN)/(0.9F*16383)+PMIN)
#define KPA_OF(x)           (PSI_OF(x)*6.895F)
#define TEMP_OF(x)          ((x*200.0F)/2047-50)

#define P_ACC               ((PMAX-PMIN)*0.0025F)
#define T_ACC                (1.5F)

static handle_t msHandle;


static int ms_read(ms4525_t *m)
{
    int r;
    U8 tmp[4];
    U16 v1,v2;
    
    r = i2c_read(msHandle, MS4525_ADDR, tmp, sizeof(tmp), 1);
    if(r==0 && m) {
        v1  = ((tmp[0]&0x3f)<<8) | tmp[1];
        v2 = (tmp[2]>>5)<<8 | tmp[2]<<3 | tmp[3]>>5;
        
        m->pres = KPA_OF(v1);
        m->temp = TEMP_OF(v2);
    }
    
    return r;
}


int ms4525_init(void)
{
#if 0    
    gpio_pin_t pin=MS4525_INT_PIN;
    gpio_init(&pin, MODE_IT_RISING);
    
    gpio_set_callback(&pin, fn);
    gpio_irq_en(&pin, 1);
#endif
    
    extern handle_t i2c1Handle;
    msHandle = i2c1Handle;
    
    //ms4525_test();
    
    return 0;
}


int ms4525_deinit(void)
{
    return 0;
}


int ms4525_get(ms4525_t *m)
{
    return ms_read(m);
}


void ms4525_test(void)
{
    int r,c=0;
    ms4525_t m;
    
    while(1) {
        r = ms_read(&m);
        if(r==0) {
            c++;
        }
        delay_ms(50); 
    }
}



