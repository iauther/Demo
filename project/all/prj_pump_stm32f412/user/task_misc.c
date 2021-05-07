#include "task.h"                          // CMSIS RTOS header file
#include "paras.h"
#include "queue.h"
 

#define TIMER_MS            200         //ms

static handle_t mq=NULL;
static void misc_tmr_callback(void *arg)
{
    task_msg_post(TASK_MISC, EVT_EEPROM, 0, NULL, 0);
}


void task_misc_fn(void *arg)
{
    int r;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    tmrId = osTimerNew(misc_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
    h->running = 1;
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            
            switch(e.evt) {
                case EVT_EEPROM:
                {
                    //paras_write_node();
                    //
                }
                break;
                
                default:
                break;
            }
        }
    }
}






