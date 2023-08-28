#include "net.h"
#include "log.h"
#include "dal.h"
#include "rs485.h"
#include "mem.h"
#include "pkt.h"
#include "comm.h"
#include "json.h"
#include "lock.h"
#include "dal_delay.h"
#include "paras.h"
#include "cmd.h"


#ifdef OS_KERNEL

#include "task.h"


tasks_handle_t tasksHandle={0};


static void comm_tmr_callback(void *arg)
{
    task_post(TASK_COMM_RECV, NULL, EVT_TIMER, 0, NULL, 0);
}


static int rs485_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    task_post(TASK_COMM_RECV, NULL, EVT_RS485, 0, NULL, 0);
    
    return 0;
}



static int log_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int  r=0;
    cmd_t cmd;
    
    //LOGD("&&: %s", data);
    r = cmd_get((char*)data, &cmd);
    if(r==0) {
        task_post(TASK_COMM_RECV, NULL, EVT_LOG, 0, &cmd, sizeof(cmd));
    }
    else {
        node_t node={data, len, len};
        task_post(TASK_COMM_RECV, NULL, EVT_COMM, 0, &node, sizeof(node));
    }
    
    return r;
}

////////////////////////////////////////////////////////

static int hw_init(void)
{
    int r=0;
    
    dal_set_priority();
    log_set_callback(log_recv_callback);
    
    lock_staic_init();
    json_init();
    paras_load();
    mem_init();
    
    
    //ec_init();
    
    return r;
}



static void start_task(int taskid, osPriority_t prio, int stkSize, osThreadFunc_t fn, int msgCnt, void *arg, int runNow)
{
    task_attr_t attr={0};
    
    attr.func = fn,
    attr.arg = arg,
    attr.nMsg = msgCnt,
    attr.taskID = taskid,
    attr.priority = prio;//osPriorityNormal,
    attr.stkSize = stkSize,
    attr.runNow  = runNow,
    task_new(&attr);
}

static void start_tasks(void)
{
    start_task(TASK_DATA_CAP,   osPriorityNormal, 2048,  task_data_cap_fn,   5, NULL, 1);
    start_task(TASK_COMM_SEND,   osPriorityNormal, 2048,  task_comm_send_fn,   5, NULL, 1);
    //start_task(TASK_POLLING,    osPriorityNormal, 1024,  task_polling_fn,    5, NULL, 1);
    
    task_simp_new(task_data_proc_fn, 2048, NULL, NULL);
    //task_simp_new(task_comm_send_fn, 2048, NULL, NULL);
    task_simp_new(task_mqtt_fn, 2048, NULL, NULL);
    //task_simp_new(task_nvm_fn,  1024, NULL, NULL);
    
    tasksHandle.hcom = comm_init(PORT_UART, log_recv_callback);
    
    //task_tmr_start(TASK_COMM_RECV, comm_tmr_callback, NULL, 200, TIME_INFINITE);
}

//////////////////////////////////////////////////////////////////////////////////////////////



static void comm_recv_init(void)
{
    hw_init();
    start_tasks();
}

void task_comm_recv_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    
    LOGD("_____ task comm recv running\n");
    
    comm_recv_init();
    while(1) {
        r = task_recv(TASK_COMM_RECV, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                                
                case EVT_COMM:
                {
                    node_t *nd=(node_t*)e.data;
                    err = comm_recv_proc(tasksHandle.hcom, e.arg, nd->buf, nd->dlen);
                }
                break;
                
                case EVT_RS485:
                {
                    
                }
                break;
                
                case EVT_LOG:
                {
                    cmd_t *cmd=(cmd_t*)e.data;
                    
                    
                    switch(cmd->ID) {
                        case CMD_USER:
                        {
                            char tmp[40];
                            static int cnt=0;
                            
                            LOGD("____ CMD_KEY\n");
                            
                            //sprintf(tmp, "task send %d", cnt++);
                            //ecxxx_send(PROTO_MQTT, tmp, strlen(tmp), 0);
                        }
                        break;
                    }
                }
                break;
                
                case EVT_TIMER:
                {
                    LOGD("____ timer\n");
                }
                break;
                
            }
        }
        
        task_yield();
    }
}

#endif


