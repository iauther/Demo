#include "task.h"                          // CMSIS RTOS header file
#include "log.h"
#include "error.h"
#include "power.h"
#include "lcd.h"
#include "font.h"
#include "drv/adc.h"
#include "drv/gpio.h"
#include "myCfg.h"

#define VOLT_MAX    10
#define ADC2VOLT(adc)   (((adc)*3.3F)/0x0fff)


typedef struct {
    F32 v1;
    F32 v2;
}volt_t;


#define ADC_MAX    3
handle_t adcHandle[ADC_MAX]={NULL};

void my_init(void)
{
    int i;
    adc_cfg_t ac[ADC_MAX]={
        {.pin=VOLT_ADC1},
        {.pin=VOLT_ADC2},
        {.pin=VOLT_ADC3},
    };
    lcd_cfg_t lc={LCD_COLOR, LCD_WIDTH, LCD_HEIGHT};
    
    lcd_init(&lc);
    lcd_fill(COLOR_BLUE);
    lcd_set_backlight(1);
    
    for(i=0; i<ADC_MAX; i++) {
        adcHandle[i] = adc_init(&ac[i]);
    }
}




static void read_volt(volt_t *volt)
{
    int i;
    F32 v;
    U32 adc[ADC_MAX];
    
    for(i=0; i<ADC_MAX; i++) {
        v = adc_read(adcHandle[i]);
        adc[i] = ADC2VOLT(v);
    }
    
}



static void show_volt(U8 idx, volt_t *volt)
{
    U8  tmp[50];
    U16 color,bgcolor;
    //font_info_t info=font_get_info(FONT_16);;
    
    bgcolor = COLOR_BLUE;
    {
        sprintf((char*)tmp, "X%02d: %0.02f", idx+1, volt->v1);
        color = (volt->v1>=3.9F)?COLOR_WHITE:COLOR_RED;
        //lcd_draw_string(30, 20+idx*(info.height+4), 80, info.height, tmp, FONT_16, color, bgcolor, HORIZONTAL_LEFT, HORIZONTAL_CENTER);
    }
    
    {
        sprintf((char*)tmp, "X%02d: %0.02f", idx+10, volt->v2);
        color = (volt->v2>=3.9F)?COLOR_WHITE:COLOR_RED;
        //lcd_draw_string(130, 20+idx*(info.height+4), 80, info.height, tmp, FONT_16, color, bgcolor, HORIZONTAL_LEFT, HORIZONTAL_CENTER);
    }
}
static void refresh_volt(void)
{
    U8 i;
    static volt_t vt;
    
    for(i=0; i<VOLT_MAX; i++) {
        //delay_ms(50);
        read_volt(&vt);
        show_volt(i, &vt);
    }
}




void test_main(void *arg)
{
    task_handle_t *h=(task_handle_t*)arg;
    
    my_init();
    while(1) {
        
        refresh_volt();
        //delay_ms(500);
        
        
    }
}


