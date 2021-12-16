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


extern handle_t adc1Handle,adc2Handle;

typedef struct {
    F32   v1;
    F32   v2;
}volt_t;
static volt_t voltData[VOLT_MAX]={0};

static void test_tmr_callback(void *arg)
{
    task_msg_post(TASK_COM, EVT_TIMER, 0, NULL, 0);
}
static void test_rx_callback(U8 *data, U16 len)
{
    task_msg_post(TASK_COM, EVT_COM, 0, data, len);
}
static void tmr_init(void)
{
    osTimerId_t tmrId;
    
    tmrId = osTimerNew(test_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, 100);
}




static void io_select(U8 v)
{
    int i;
    gpio_pin_t p[4]={VOLT_S0_PIN, VOLT_S1_PIN, VOLT_S2_PIN, VOLT_S3_PIN};
    
    for(i=0; i<4; i++) {
        if(v&(1<<i)) {
            gpio_set_hl(&p[i], 1);
        }
        else {
            gpio_set_hl(&p[i], 0);
        }
    }  
}


static F32 adc_to_volt(U32 adc)
{
    return (adc*3.3F/0x0fff);
}
static void read_volt(F32 *volt)
{
    U8 i;
    U32 adc1,adc2;
    
    for(i=0; i<VOLT_MAX; i++) {
        
        io_select(i);
        adc1 = adc_read(adc1Handle);
        adc2 = adc_read(adc2Handle);
        
        voltData[i].v1 = ADC2VOLT(adc1);
        voltData[i].v2 = ADC2VOLT(adc2);
    }
}


static void ui_init(void)
{
    lcd_cfg_t cfg={LCD_COLOR, LCD_WIDTH, LCD_HEIGHT};
        
    lcd_init(&cfg);
    lcd_clear(COLOR_BLACK);
    lcd_draw_string(50, 100, 200, 60, (U8*)"HELLO!", FONT_16, COLOR_WHITE, COLOR_BLACK, HORIZONTAL_LEFT, HORIZONTAL_CENTER);
    lcd_set_backlight(1);
}
static void ui_show_volt(volt_t *volt)
{
    int i;
    U8  tmp[40];
    
    for(i=0; i<VOLT_MAX/2; i++) {
        sprintf((char*)tmp, "volt%d: %f       volt%d: %f", i, volt[i].v1, i+1, volt[i].v2);
        lcd_draw_string(40+i*40, 20, 40, 60, tmp, FONT_16, COLOR_WHITE, COLOR_BLACK, HORIZONTAL_LEFT, HORIZONTAL_CENTER);
        
    }
}
static void volt_update(void)
{
    read_volt(voltData);
    ui_show_volt(voltData);
}




void task_test_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    ui_init();
    tmr_init();
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    volt_update();
                }
                break;                
                
                default:
                continue;
            }
            
            
        }
    }
}



