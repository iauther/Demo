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


handle_t adc1Handle=NULL;
static void show_volt(U8 idx, volt_t *volt);

static void pwr_en(U8 on)
{
    gpio_pin_t pwrPin=POWER_EN;
    
    gpio_init(&pwrPin, MODE_OUTPUT);
    gpio_set_hl(&pwrPin, on);
}
static void ios_init(void)
{
    int i;
    gpio_pin_t iosPIn[4]={VOLT_S0_PIN,VOLT_S1_PIN,VOLT_S2_PIN,VOLT_S3_PIN};
    
    for(i=0; i<4; i++) {
        gpio_init(&iosPIn[i], MODE_OUTPUT);
    }
}


void my_init(void)
{
    int i;
    adc_cfg_t ac1={.pin=VOLT_ADC1};
    lcd_cfg_t lc={LCD_COLOR, LCD_WIDTH, LCD_HEIGHT};
    
    lcd_init(&lc);
    lcd_fill(COLOR_BLUE);
    lcd_set_backlight(1);
    
    ios_init();
    adc1Handle = adc_init(&ac1);
}
static void sel_volt(U8 idx)
{
    int i;
    gpio_pin_t p[4]={VOLT_S0_PIN, VOLT_S1_PIN, VOLT_S2_PIN, VOLT_S3_PIN};
    
    for(i=0; i<4; i++) {
        if(idx&(1<<i)) {
            gpio_set_hl(&p[i], 1);
        }
        else {
            gpio_set_hl(&p[i], 0);
        }
    }  
}


static F32 revise_volt(F32 v)
{
    F32 x,f;
    
    if(v>1.0F) {
        x = v*2+0.4F;
    }
    else {
        x = v;
    }
    
    return x;
}

static void read_volt(volt_t *volt)
{
    F32 v1,v2;
    U32 adc1,adc2;
    
    adc1 = adc_read(adc1Handle, ADC_CHANNEL_0);
    adc2 = adc_read(adc1Handle, ADC_CHANNEL_1);
    
    v1 = ADC2VOLT(adc1);
    v2 = ADC2VOLT(adc2);
    
    volt->v1 = revise_volt(v1);
    volt->v2 = revise_volt(v2);
}



static void show_volt(U8 idx, volt_t *volt)
{
    U8  tmp[50];
    U16 color,bgcolor;
    font_info_t info=font_get_info(FONT_16);;
    
    bgcolor = COLOR_BLUE;
    {
        sprintf((char*)tmp, "X%02d: %0.02f", idx+1, volt->v1);
        color = (volt->v1>=3.9F)?COLOR_WHITE:COLOR_RED;
        lcd_draw_string(30, 20+idx*(info.height+4), 80, info.height, tmp, FONT_16, color, bgcolor, HORIZONTAL_LEFT, HORIZONTAL_CENTER);
    }
    
    {
        sprintf((char*)tmp, "X%02d: %0.02f", idx+10, volt->v2);
        color = (volt->v2>=3.9F)?COLOR_WHITE:COLOR_RED;
        lcd_draw_string(130, 20+idx*(info.height+4), 80, info.height, tmp, FONT_16, color, bgcolor, HORIZONTAL_LEFT, HORIZONTAL_CENTER);
    }
}
static void refresh_volt(void)
{
    U8 i;
    static volt_t vt;
    
    for(i=0; i<VOLT_MAX; i++) {
        sel_volt(i);
        delay_ms(500);
        
        read_volt(&vt);
        show_volt(i, &vt);
    }
}



#ifdef OS_KERNEL
void task_test_fn(void *arg)
{
    task_handle_t *h=(task_handle_t*)arg;
    
    my_init();
    while(1) {
        
        refresh_volt();
        //delay_ms(500);
        
        
    }
}

#endif

