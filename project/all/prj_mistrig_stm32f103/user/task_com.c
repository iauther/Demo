#include "task.h"                          // CMSIS RTOS header file
#include "drv/uart.h"



#define TIMER_MS     2000


static U32 tmr_cnt=0;

static void tcp232_rx_callback(U8 *data, U16 len)
{
    static int r=0;
    r = task_msg_post(TASK_COM, EVT_COM, 0, data, len);
    r++;
}



static void timer_proc(void)
{
    static int cnt=0;
   
}

static void tmr_callback(void *arg)
{
    task_msg_post(TASK_COM, EVT_TIMER, 0, NULL, 0);
}
static void timer_start(void)
{
    osTimerId_t tmrId;
    
    tmrId = osTimerNew(tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
}



void task_com(void *arg)
{
    int r;
    evt_t e;
    task_handle_t *h=(task_handle_t*)arg;
    
    
    timer_start();
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            
            switch(e.evt) {
                case EVT_COM:
                {
                    //ph = (tcp232_hdr_t*)e.data;
                    //tcp232_write(e.data, e.dLen);
                }
                break;
                
                case EVT_TIMER:
                {
                    timer_proc();
                }
                break;
                
                default:
                break;
            }
        }
        
    }
}



