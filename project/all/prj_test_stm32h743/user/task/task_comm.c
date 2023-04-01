#include "net.h"
#include "com.h"
#include "log.h"
#include "cap.h"
#include "task.h"
#include "list.h"


typedef struct {
    handle_t    hcom;
    void        *netAddr;
}my_handle_t;


static my_handle_t myHandle={0};
////////////////////////////////////////////
static int com_rx_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int r=-1;
    
    switch(evt) {
        case NET_EVT_NEW_CONN:
        myHandle.netAddr = addr;
        //r = task_tmr_start(TASK_COMM, com_tmr_callback, addr, PERIOD_TIME, TIME_INFINITE);
        break;
        
        case NET_EVT_DIS_CONN:
        myHandle.netAddr = NULL;
        //r = task_tmr_stop(TASK_COMM);
        break;
        
        case NET_EVT_DATA_IN:
        r = task_post(TASK_COMM, addr, EVT_COM, evt, data, len);
        break;
    }
    
    return r;
}

#ifdef OS_KERNEL
void task_comm_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;

    myHandle.hcom = com_init(PORT_ETH, com_rx_callback);
    cap_start();
    
    while(1) {
        r = task_recv(TASK_COMM, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                                
                case EVT_COM:
                {
                    err = com_proc(myHandle.hcom, e.addr, e.data, e.dLen);
                }
                break;
            }
        }
    }
}
#endif


