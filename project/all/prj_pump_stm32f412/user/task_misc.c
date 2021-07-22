#include "task.h"                          // CMSIS RTOS header file
#include "paras.h"
#include "queue.h"
#include "wdog.h"
 

#define TIMER_MS                        200         //ms
#define PAD_POWERON_DELAY_MS            1000

static handle_t mq=NULL;


static U8 pad_poweron_flag=0;
static U32 pad_poweron_delay=0;
static void pad_poweron_check(void)
{
    if(!pad_poweron_flag) {
        pad_poweron_delay += ACK_POLL_MS;
        if(pad_poweron_delay>=TIMER_MS) {
            pad_poweron_flag = 1;
            pad_poweron_delay = 0;
        }
    }
}

static int my_qiterater(handle_t h, int index, void *p1, void *p2)
{
    node_t *n1=((node_t*)p1)->ptr;
    node_t *n2=((node_t*)p2)->ptr;
    
    if(n1->ptr==n2->ptr && n1->len==n2->len) {
        queue_iterate_quit(mq);
        //memcpy(n2->ptr, n1->ptr, n1->len);
        return index;
    }
    
    return -1;
}

static void misc_tmr_callback(void *arg)
{
    task_msg_post(TASK_MISC, EVT_TIMER, 0, NULL, 0);
}

static void eeprom_proc(evt_t *e)
{
    int r;
    node_t n,node;
    
    node.ptr = &n;
    node.len = sizeof(n);
    if(e->type==0) {
        r = queue_get(mq, &node, NULL);
        if(r==0) {
            paras_write_node(&n);
        }
    }
}



void task_misc_fn(void *arg)
{
    int r;
    evt_t e;
    
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    mq = queue_init(10, sizeof(node_t));
    tmrId = osTimerNew(misc_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            
            switch(e.evt) {
                case EVT_TIMER:
                {
                    eeprom_proc(&e);
                    //pad_poweron_check();
                    wdog_feed();
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




