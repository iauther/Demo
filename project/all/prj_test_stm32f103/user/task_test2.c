#include "task.h"                          // CMSIS RTOS header file
#include "log.h"
#include "error.h"
#include "lcd.h"
#include "font.h"
#include "drv/gpio.h"
#include "myCfg.h"


extern handle_t adc1Handle,adc2Handle;


#define LEAD_SW_TIME    3000
static U8 cur_lead_idx=0;
enum {
    LEAD_I=0,
    LEAD_II,
    LEAD_III,
    LEAD_V1,
    LEAD_V2,
    LEAD_V3,
    LEAD_V4,
    LEAD_0,
    
    LEAD_MAX
};


static void io_init(void)
{
    int i;
    gpio_pin_t p[]={
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {GPIOA, GPIO_PIN_0},
        {0,0},
    };
    
    for(i=0;; i++) {
        if(p[i].grp==0) {
            break;
        }
        
        gpio_init(&p[i], MODE_OUTPUT);
        
    }
    
}

static void gpio_set()
{
    
}




static void lead_switch(U8 lead)
{
    gpio_pin_t p;
    
    switch(lead) {
        
        case LEAD_0:
        {
            p.grp = GPIOA;
            p.pin = GPIO_PIN_0;
            gpio_set_hl(&p, 1);
            
            p.grp = GPIOB;
            p.pin = GPIO_PIN_2;
            gpio_set_hl(&p, 1);
            
            
            
        }
        break;
        
        case LEAD_I:
        break;
        
        case LEAD_II:
        break;
        
        case LEAD_III:
        break;
        
        case LEAD_V1:
        break;
        
        case LEAD_V2:
        break;
        
        case LEAD_V3:
        break;
        
        case LEAD_V4:
        break;
    }
    
    
    
}
/////////////////////////////////////////////////////////


static void test_tmr_callback(void *arg)
{
    task_msg_post(TASK_COM, EVT_TIMER, 0, NULL, 0);
}
static void test_rx_callback(U8 *data, U16 len)
{
    task_msg_post(TASK_COM, EVT_COM, 0, data, len);
}
static void task_test_init(void)
{
    osTimerId_t tmrId;
    
    tmrId = osTimerNew(test_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, LEAD_SW_TIME);
}





void task_test_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    task_test_init();
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    lead_switch(cur_lead_idx);
                    cur_lead_idx = (cur_lead_idx+1)%LEAD_MAX;
                }
                break;                
                
                default:
                continue;
            }
            
            
        }
    }
}



