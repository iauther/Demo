#include "net.h"
#include "com.h"
#include "log.h"
#include "cap.h"
#include "task.h"
#include "list.h"
#include "fs.h"


comm_handle_t commHandle={0};


#ifdef OS_KERNEL
static void start_task(int taskid, osPriority_t prio, int stkSize, osThreadFunc_t fn, int msgCnt, void *arg)
{
    task_attr_t attr={0};
    
    attr.func = fn,
    attr.arg = arg,
    attr.nMsg = msgCnt,
    attr.taskID = taskid,
    attr.priority = prio;//osPriorityNormal,
    attr.stkSize = stkSize,
    attr.runNow  = 1,
    task_new(&attr);
}

static void start_tasks(void)
{
    start_task(TASK_DATACAP,  osPriorityNormal, 1024, task_datacap_fn,  5, NULL);
    start_task(TASK_DATAPROC, osPriorityNormal, 1024, task_dataproc_fn, 5, NULL);
    //start_task(TASK_POLLING,  osPriorityNormal, 512,  task_polling_fn,  5, NULL);
}

static int com_rx_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int r=-1;
    
    switch(evt) {
        case NET_EVT_NEW_CONN:
        commHandle.netAddr = addr;
        //r = task_tmr_start(TASK_COMM, com_tmr_callback, addr, PERIOD_TIME, TIME_INFINITE);
        break;
        
        case NET_EVT_DIS_CONN:
        commHandle.netAddr = NULL;
        //r = task_tmr_stop(TASK_COMM);
        break;
        
        case NET_EVT_DATA_IN:
        r = task_post(TASK_COMM, addr, EVT_COM, evt, data, len);
        break;
    }
    
    return r;
}
//////////////////////////////////////////////////////////////////////////////////////////////

static void comm_init(void)
{
    extern void cap_Init(void);
    
    cap_Init();
    commHandle.hcom = com_init(PORT_ETH, com_rx_callback);
    
}

void task_comm_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    
    fs_test();
    comm_init();
    start_tasks();
    
    while(1) {
        r = task_recv(TASK_COMM, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                                
                case EVT_COM:
                {
                    err = com_proc(commHandle.hcom, e.arg, e.data, e.dLen);
                }
                break;
            }
        }
    }
}
#endif


