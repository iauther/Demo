#include "dal/adc.h"
#include "eth\eth.h"
#include "lwip.h"
#include "com.h"
#include "log.h"
#include "msg.h"
#include "task.h"



#define POLL_MS    500

#ifdef OS_KERNEL

static handle_t comHandle=NULL;
static void com_tmr_callback(void *arg)
{
    task_post(TASK_COM, NULL, EVT_TIMER, 0, NULL, 0);
}
static int com_rx_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    return task_post(TASK_COM, addr, EVT_COM, evt, data, len);
}



static void task_com_init(void)
{
    //log_init();
    //urt_init();
    //adc_init();
    
    comHandle = com_init(PORT_ETH, com_rx_callback);
    //com_send_paras(1);
    
    //task_tmr_start(TASK_COM, com_tmr_callback, POLL_MS, TIME_INFINITE);
}


static void start_others(void)
{
    task_attr_t attr={0};
    
    attr.func = task_poll_fn,
    attr.arg = NULL,
    attr.nMsg = 5,
    attr.taskID = TASK_POLL,
    attr.priority = osPriorityNormal,
    attr.stkSize = 1024,
    attr.runNow  = 1,
    task_new(&attr);
}


void task_com_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    
    task_com_init();
    start_others();
    
    while(1) {
        r = task_recv(TASK_COM, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    //err = com_ack_poll();    
                }
                break;
                
                case EVT_COM:
                {
                    err = com_proc(comHandle, e.addr, e.data, e.dLen);
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


