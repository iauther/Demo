#include "incs.h"

#define POLL_MS    500

#ifdef OS_KERNEL


static int comID=0;

static void com_tmr_callback(void *arg)
{
    task_post(comID, EVT_TIMER, 0, NULL, 0);
}
static void com_rx_callback(U8 *data, U16 len)
{
    task_post(comID, EVT_COM, 0, data, len);
}
static void task_com_init(void)
{
    osTimerId_t tmrId;
    
    tmrId = osTimerNew(com_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, POLL_MS);
    
    //urt_init();
    //adc_init();
    //adc2_init();
    
    //com_init(com_rx_callback, POLL_MS);
    //com_send_paras(1);
}



void task_com_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    //task_com_init();
    net2_test();
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    //err = com_ack_poll();    
                }
                break;
                
                case EVT_COM:
                {
                    //err = com_data_proc(e.data, e.dLen);
                }
                break;
                
                default:
                continue;
            }
            
            if(err==0) {
                //led_set_color(GREEN);
            }
            else if(err && !(err>=ERROR_PKT_TYPE && err<ERROR_FW_INFO_VERSION)) {
                //led_set_color(RED);
            }
        }
    }
}
#endif


