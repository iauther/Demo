#include "task.h"                          // CMSIS RTOS header file
#include "log.h"
#include "error.h"
#include "power.h"
#include "lcd.h"
#include "font.h"
#include "drv/adc.h"
#include "drv/gpio.h"
#include "myCfg.h"

#define ADC_MAX     3
#define VOLT_MAX    10
#define ADC2VOLT(adc)   (((adc)*3.3F)/0x0fff)


typedef struct {
    F32 v[ADC_MAX];
}volt_t;



handle_t adcHandle[ADC_MAX]={NULL};

void my_init(void)
{
    int i;
    adc_cfg_t ac[ADC_MAX]={
        {.pin=VOLT_ADC1},
        {.pin=VOLT_ADC2},
        {.pin=VOLT_ADC3},
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




void test_main(void *arg)
{
    //task_handle_t *h=(task_handle_t*)arg;
    
    my_init();
    while(1) {
        
        
        //delay_ms(500);
        
        
    }
}


