#include "task.h"                          // CMSIS RTOS header file

#ifdef OS_KERNEL

#define TIMER_MS            500         //ms

static handle_t mq=NULL;


static void misc_tmr_callback(void *arg)
{
    task_msg_post(TASK_MISC, EVT_TIMER, 0, NULL, 0);
}


void task_test(void *arg)
{
    int r;
    evt_t e;
    node_t node,n;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    tmrId = osTimerNew(misc_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            
            switch(e.evt) {
                case EVT_TIMER:
                {
                    
                    
                }
                break;
                
                default:
                break;
            }
        }
    }
    
}

#endif




