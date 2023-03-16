#include "eth\eth.h"
#include "task.h"


#define POLL_MS    100
#ifdef OS_KERNEL

static U32 tmrCount=0;
static void com_tmr_callback(void *arg)
{
    task_post(TASK_POLL, NULL, EVT_TIMER, 0, NULL, 0);
}


void task_poll_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    task_tmr_start(TASK_POLL, com_tmr_callback, NULL, POLL_MS, TIME_INFINITE);
    while(1) {
        r = task_recv(TASK_POLL, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    tmrCount++;
                    //eth_link_check();
                    
                    if(tmrCount%10==0) {
                        
                    }  
                }
                break;                
                
                default:
                continue;
            }
        }
    }
}
#endif


