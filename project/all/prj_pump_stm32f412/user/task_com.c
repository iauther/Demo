#include "task.h"                          // CMSIS RTOS header file
#include "log.h"
#include "error.h"
#include "data.h"
#include "com.h"
#include "led.h"
#include "power.h"
#include "paras.h"



static void com_tmr_callback(void *arg)
{
    task_msg_post(TASK_COM, EVT_TIMER, 0, NULL, 0);
}
static void com_rx_callback(U8 *data, U16 len)
{
    task_msg_post(TASK_COM, EVT_COM, 0, data, len);
}
static void task_com_init(void)
{
    osTimerId_t tmrId;
    
    tmrId = osTimerNew(com_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, ACK_POLL_MS);
    
    com_init(com_rx_callback, ACK_POLL_MS);
    com_send_paras(1);
}


void task_com_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    task_com_init();
    task_start_others();
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    err = com_ack_poll();    
                }
                break;
                
                case EVT_COM:
                {
                    err = com_data_proc(e.data, e.dLen);
                }
                break;
                
                default:
                continue;
            }
            
            if(err) {
                //do something here
                //led_set_color(RED);
                err = 0;
            }
        }
    }
}



