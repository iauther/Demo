#include "task.h"                          // CMSIS RTOS header file
#include "log.h"
#include "error.h"
#include "com.h"

#define TIMER_MS            1000

static void com_tmr_callback(void *arg)
{
    task_msg_post(TASK_COM, EVT_TIMER, 0, NULL, 0);
}
static void com_rx_callback(U8 *data, U16 len)
{
    int r;
    r = task_msg_post(TASK_COM, EVT_COM, 0, data, len);
    r = 3;
}



void task_com_fn(void *arg)
{
    int r;
    U8  err;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    
    tmrId = osTimerNew(com_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
    h->running = 1;
    
    //jump_to_boot();
    com_init(com_rx_callback, TIMER_MS);
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    err = com_loop_check(1);
                }
                break;
                
                case EVT_COM:
                {
                    err = com_data_proc(e.data, e.dLen);
                }
                break;
                
                default:
                break;
            }
            
            if(err) {
                //do something here
                //led_set_color();
                err = 0;
            }
        }
    }
}



