#include "task.h"                          // CMSIS RTOS header file
#include "tcp232.h"



#define TIMER_MS     2000
typedef struct {
    U8      magic;
    U8      flag;
}tcp232_hdr_t;


static U32 tmr_cnt=0;

static void tcp232_rx_callback(U8 *data, U16 len)
{
    static int r=0;
    r = task_msg_post(TASK_TCP232, EVT_COM, 0, data, len);
    r++;
}



static void timer_proc(void)
{
    char tmp[50];
    static int cnt=0;
    
    sprintf(tmp, "%s%d", "hello ", cnt++);
    tcp232_write((U8*)tmp, strlen(tmp));
}

static void tmr_callback(void *arg)
{
    task_msg_post(TASK_TCP232, EVT_TIMER, 0, NULL, 0);
}
static void timer_start(void)
{
    osTimerId_t tmrId;
    
    tmrId = osTimerNew(tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
}


static void tcp232_set(void *to, char *value)
{
    strcpy((char*)to, value);
}


void task_tcp232(void *arg)
{
    int r;
    evt_t e;
    tcp232_hdr_t *ph;
    tcp232_net_t set;
    task_handle_t *h=(task_handle_t*)arg;
    
    tcp232_init(tcp232_rx_callback);
    
    tcp232_set(set.mode,        "static");
    tcp232_set(set.dns,         "192.168.2.1");
    tcp232_set(set.cliAddr.ip,  "192.168.2.147");
    set.cliAddr.port = 8888;
    tcp232_set(set.svrAddr.ip,  "192.168.2.13");
    set.svrAddr.port = 9999;
    tcp232_set(set.mask,        "255.255.255.0");
    tcp232_set(set.gateway,     "192.168.2.1");
    tcp232_set_net(&set);
    
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



