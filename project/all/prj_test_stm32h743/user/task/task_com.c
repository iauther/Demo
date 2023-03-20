#include "dal/adc.h"
#include "eth\eth.h"
#include "net.h"
#include "lwip.h"
#include "com.h"
#include "log.h"
#include "msg.h"
#include "task.h"


#define PERIOD_TIME    500

#ifdef OS_KERNEL

#define NODE_LEN    (ADC_CAP_LEN)

handle_t comHandle=NULL;
static void *netAddr=NULL;


static void com_tmr_callback(void *arg)
{
    task_handle_t *h=arg;
    
    task_post(TASK_COM, h->tmrArg, EVT_TIMER, 0, NULL, 0);
}
static void adc_data_callback(U8 *data, U32 len)
{
    if(netAddr) {
        
        node_t node;
        
        node.ptr = data;
        node.len = len;
        //task_post(TASK_COM, NULL, EVT_ADC, 0, &node, sizeof(node));
    }
}



static int com_rx_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int r=-1;
    
    switch(evt) {
        case NET_EVT_NEW_CONN:
        netAddr = addr;
        r = task_tmr_start(TASK_COM, com_tmr_callback, addr, PERIOD_TIME, TIME_INFINITE);
        break;
        
        case NET_EVT_DIS_CONN:
        netAddr = NULL;
        r = task_tmr_stop(TASK_COM);
        break;
        
        case NET_EVT_DATA_IN:
        r = task_post(TASK_COM, addr, EVT_COM, evt, data, len);
        break;
    }
    
    return r;
}




static void start_others(void)
{
    task_attr_t attr={0};
    
    attr.func = task_poll_fn,
    attr.arg = NULL,
    attr.nMsg = 5,
    attr.taskID = TASK_POLL,
    attr.priority = osPriorityNormal,
    attr.stkSize = 512,
    attr.runNow  = 1,
    task_new(&attr);
}


void task_com_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    adc_cfg_t cfg;
    
    log_init();
    LOGI("______ 8888\n");
    
    cfg.dual = 1;//ADC_DUAL_MODE;
    cfg.mode = MODE_DMA;
    cfg.samples = 400;
    cfg.callback = adc_data_callback;
    adc_init(&cfg);
    
    comHandle = com_init(PORT_ETH, com_rx_callback);
    //start_others();
    
    while(1) {
        r = task_recv(TASK_COM, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    static int test_cnt=0;
                    char tmp[20]={0};
                    
                    sprintf(tmp, "test %d\n", test_cnt++);
                    com_send_data(comHandle, e.addr, TYPE_TEST, 0, tmp, strlen(tmp)+1);   
                }
                break;
                
                case EVT_ADC:
                {
                    if(netAddr) {
                        node_t *nd=(node_t*)e.data;
                        err = com_send_data(comHandle, netAddr, EVT_ADC, 0, nd->ptr, nd->len);
 
                    }
                }
                break;
                
                case EVT_COM:
                {
                    err = com_proc(comHandle, e.addr, e.data, e.dLen);
                }
                break;                
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


