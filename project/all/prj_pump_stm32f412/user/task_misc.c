#include "task.h"                          // CMSIS RTOS header file
#include "paras.h"
#include "queue.h"
 

#define TIMER_MS            200         //ms

static handle_t mq=NULL;



static int my_qiterater(handle_t h, int index, void *p1, void *p2)
{
    node_t *n1=((node_t*)p1)->ptr;
    node_t *n2=((node_t*)p2)->ptr;
    
    if(n1->ptr==n2->ptr && n1->len==n2->len) {
        queue_iterate_quit(mq);
        //memcpy(n2->ptr, n1->ptr, n1->len);
    }
    
    return 0;
}

static void misc_tmr_callback(void *arg)
{
    task_msg_post(TASK_MISC, EVT_EEPROM, 0, NULL, 0);
}


void task_misc_fn(void *arg)
{
    int r;
    evt_t e;
    node_t node,n;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    mq = queue_init(10, sizeof(node_t));
    node.ptr = &n;
    node.len = sizeof(n);
    
    tmrId = osTimerNew(misc_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            
            switch(e.evt) {
                case EVT_EEPROM:
                {
                    if(e.type==0) {
                        r = queue_get(mq, &node, NULL);
                        if(r==0) {
                            paras_write_node(&n);
                        }
                    }
                    else if(e.type==1) {
                        
                    }
                    else if(e.type==2) {
                        
                    }
                    
                }
                break;
                
                default:
                break;
            }
        }
    }
    
    queue_free(&mq);
}


int task_misc_save_paras(node_t *n)
{
    node_t node={n, sizeof(node_t)};
    return queue_put(mq, &node, my_qiterater);
}




