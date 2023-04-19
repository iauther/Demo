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
    //start_task(TASK_COMM_SEND,  osPriorityNormal, 1024,  task_comm_send_fn,  5, NULL);
    start_task(TASK_DATA_CAP,  osPriorityNormal, 1024, task_data_cap_fn,  5, NULL);
    start_task(TASK_DATA_PROC, osPriorityNormal, 1024, task_data_proc_fn, 5, NULL);
    //start_task(TASK_POLLING,  osPriorityNormal, 512,  task_polling_fn,  5, NULL);
}

static int comm_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int r=-1;
    
    switch(evt) {
        case NET_EVT_NEW_CONN:
        commHandle.netAddr = addr;
        //r = task_tmr_start(TASK_COMM_RECV, com_tmr_callback, addr, PERIOD_TIME, TIME_INFINITE);
        break;
        
        case NET_EVT_DIS_CONN:
        commHandle.netAddr = NULL;
        //r = task_tmr_stop(TASK_COMM_RECV);
        break;
        
        case NET_EVT_DATA_IN:
        r = task_post(TASK_COMM_RECV, addr, EVT_COMM, evt, data, len);
        break;
    }
    
    return r;
}
//////////////////////////////////////////////////////////////////////////////////////////////

static void comm_recv_init(void)
{
    extern void cap_Init(void);
    
    cap_Init();
    commHandle.hcom = com_init(PORT_ETH, comm_recv_callback);
    
    start_tasks();
}

void task_comm_recv_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    
    fs_test();
    comm_recv_init();
    
    while(1) {
        r = task_recv(TASK_COMM_RECV, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                                
                case EVT_COMM:
                {
                    err = com_proc(commHandle.hcom, e.arg, e.data, e.dLen);
                }
                break;
            }
        }
    }
}
#endif


