#include "task.h"                          // CMSIS RTOS header file
#include "lcd.h"
#include "error.h"
#include "log.h"
#include "paras.h"
#include "myCfg.h"

#include "drv/adc.h"
#include "drv/gpio.h"
#include "drv/i2c.h"




#define ADC_MAX     3
#define VOLT_MAX    10
#define ADC2VOLT(adc)   (((adc)*3.3F)/0x0fff)


typedef struct {
    F32 v[ADC_MAX];
}volt_t;



handle_t adcHandle[ADC_MAX]={NULL};
handle_t i2c1Handle=NULL;
static void hw_i2c_init(void)
{
    i2c_cfg_t  ic;
    i2c_pin_t  p1={I2C1_SCL_PIN,I2C1_SDA_PIN};
    
    
    ic.pin= p1;
    ic.freq = I2C1_FREQ;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    i2c1Handle = i2c_init(&ic); 
}

typedef struct {
    gpio_pin_t      pin;
    U8              mode;
}gpio_init_t;
static gpio_init_t gpioInit[]={
    {},
    
};
static void hw_gpio_init(void)
{
    
}







void my_init(void)
{
    int i;
    adc_cfg_t ac[ADC_MAX]={
        //{.pin=VOLT_ADC1},
        //{.pin=VOLT_ADC2},
        //{.pin=VOLT_ADC3},
    };
    lcd_cfg_t lc={LCD_COLOR, 0, 0};
    
    
    
    
    
    
    
    
    
    
    
    
    
    lcd_init(&lc);
    lcd_fill(COLOR_BLUE);
    lcd_set_backlight(1);
    
    for(i=0; i<ADC_MAX; i++) {
        adcHandle[i] = adc_init(&ac[i]);
    }
}




static void update_volt(volt_t *volt)
{
    int i;
    F32 v;
    U32 adc[ADC_MAX];
    
    for(i=0; i<ADC_MAX; i++) {
        v = adc_read(adcHandle[i]);
        volt->v[i] = ADC2VOLT(v);
    }
    
    //lcd_draw_string(1, 2, 0, 0, tmp, 0, 0, 0, HORIZONTAL_LEFT, HORIZONTAL_CENTER);
}




void task_test(void *arg)
{
    //task_handle_t *h=(task_handle_t*)arg;
    
    my_init();
    while(1) {
        
        
        //delay_ms(500);
        
        
    }
}


