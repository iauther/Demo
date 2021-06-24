#include "drv/i2c.h"
#include "drv/gpio.h"
#include "drv/si2c.h"
#include "drv/delay.h"
#include "ms4525.h"
#include "myCfg.h"

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
    //i2c_read2(MS4525_ADDR, tmp, sizeof(tmp));
    if(r==0) {
        v1  = ((tmp[0]&0x3f)<<8) | tmp[1];
        v2 = (tmp[2]>>5)<<8 | tmp[2]<<3 | tmp[3]>>5;
        
        if(m) {
            m->pres = KPA_OF(v1);
            m->temp = TEMP_OF(v2);
        }
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




void HAL_I2C_MspInit2(I2C_HandleTypeDef* hi2c)
{
    GPIO_InitTypeDef init = {0};
    
    init.Mode = GPIO_MODE_AF_OD;
    init.Pull = GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    if(hi2c->Instance==I2C1) {
        __HAL_RCC_I2C1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        init.Pin = GPIO_PIN_6|GPIO_PIN_7;
        init.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &init);
    }
    else if(hi2c->Instance==I2C2) {
        __HAL_RCC_I2C2_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        init.Pin = GPIO_PIN_9|GPIO_PIN_10;
        init.Alternate = GPIO_AF4_I2C2;
        HAL_GPIO_Init(GPIOB, &init);
    }
}
I2C_HandleTypeDef hi2c1;
static int i2c1_init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    return -1;
  }
  return 0;

}
int i2c1_read(U16 addr, U8 *data, U32 len, U8 bStop)
{
    int r;

    //r = HAL_I2C_Master_Receive2(&ih->hi2c, (addr<<1), data, len, HAL_MAX_DELAY, bStop);
    r = HAL_I2C_Master_Receive(&hi2c1, (addr<<1), data, len, HAL_MAX_DELAY);
    
    return r;
}
static int ms4525_read(ms4525_t *m)
{
    int r;
    U8 tmp[4];
    U16 v1,v2;
    
    r = i2c1_read(MS4525_ADDR, tmp, sizeof(tmp), 1);
    if(r==0 && m) {
        v1  = ((tmp[0]&0x3f)<<8) | tmp[1];
        v2 = (tmp[2]>>5)<<8 | tmp[2]<<3 | tmp[3]>>5;
        
        m->pres = KPA_OF(v1);
        m->temp = TEMP_OF(v2);
    }
    
    return r;
}


void ms4525_test(void)
{
    int r,c=0;
    ms4525_t ms={0};
    
    //i2c1_init();
    while(1) {
        //r = ms4525_read(&ms);
        r = ms_read(&ms);
        if(r==0) {
            c++;
        }
        delay_ms(100); 
    }
}



