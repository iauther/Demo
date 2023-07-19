#include "net.h"
#include "comm.h"
#include "log.h"
#include "dal.h"
#include "rs485.h"
#include "ecxxx.h"
#include "fs.h"

#ifdef OS_KERNEL

#include "task.h"



tasks_handle_t tasksHandle={0};


static void task_mem_init(void)
{
    list_cfg_t cfg;
    
    tasksHandle.hMem = NULL;
    
    tasksHandle.capBuf.blen = SAMPLE_POINTS*4*2+32;
    tasksHandle.capBuf.buf = malloc(tasksHandle.capBuf.blen); 
    
    tasksHandle.calBuf.blen = SAMPLE_POINTS*4*2+32;
    tasksHandle.calBuf.buf = malloc(tasksHandle.calBuf.blen); 
}

static void cfg_clear(void)
{
    int i;
    
    for(i=0; i<CH_MAX; i++) {
        tasksHandle.cfg[i].chID = 0xff;
    }
}

static void task_list_cfg(void)
{
    int i;
    list_cfg_t lc;
    
    lc.max = 10;
    lc.mode = MODE_FIFO;
    lc.hmem = tasksHandle.hMem;
    for(i=0; i<CH_MAX; i++) {
        
        if(tasksHandle.oriList[i]) {
            list_free(tasksHandle.oriList[i]);
            tasksHandle.oriList[i]=NULL;
        }
        if(tasksHandle.covList[i]) {
            list_free(tasksHandle.covList[i]);
            tasksHandle.covList[i]=NULL;
        }
        
        if(tasksHandle.cfg[i].chID<CH_MAX) {
            tasksHandle.oriList[i] = list_init(&lc);
            tasksHandle.covList[i] = list_init(&lc);
        }
        else {
            tasksHandle.oriList[i] = NULL;
            tasksHandle.covList[i] = NULL;
        }
    }
    
    //lc.max = 10;
    tasksHandle.sendList = list_init(&lc);
    
    //lc.max = 10;
    tasksHandle.connList = list_init(&lc);
}




static int json_parse(char *json)
{
    int len,f=0;
    char *js=json;
    char tmp[2]={0};
    
    cfg_clear();
#if 0
    if(!js) {
        len = fs_read(FILE_CFG, NULL, 0);
        if(len<=0) {
            return -1;
        }
        js = malloc(len);
        if(!js) return -1;
        fs_read(FILE_CFG, js, len);
        
        f = 1;
    }
    else {
        len = fs_write(FILE_CFG, js, strlen(js), 0);
        len = fs_append(FILE_CFG, tmp, 2, 1);
    }
#endif
    
    //json_to_ch(js, tasksHandle.cfg, CH_MAX);
    
    if(f) free(js);
    
    return 0;
}
static void comm_tmr_callback(void *arg)
{
    task_post(TASK_COMM_RECV, NULL, EVT_TIMER, 0, NULL, 0);
}


static int rs485_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    task_post(TASK_COMM_RECV, NULL, EVT_RS485, 0, NULL, 0);
    
    return 0;
}
static int ecxxx_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    task_post(TASK_COMM_RECV, NULL, EVT_ECXXX, 0, NULL, 0);
    
    return 0;
}





static int hw_init(void)
{
    int r=0;
    
    task_mem_init();
    task_list_cfg();
    
    return r;
}



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
    start_task(TASK_DATA_CAP,   osPriorityNormal, 1024, task_data_cap_fn,  5, NULL);
    //start_task(TASK_DATA_PROC,  osPriorityNormal, 1024, task_data_proc_fn, 5, NULL);
    //start_task(TASK_POLLING,  osPriorityNormal, 512,  task_polling_fn,  5, NULL);
    //start_task(TASK_UPGRADE,  osPriorityNormal, 512,  task_upgrade_fn,  5, NULL);
    
    //task_wait(TASK_DATA_PROC);
    //LOGD("______ task wait finished\n");
    
    //tasksHandle.hcom = comm_init(PORT_NET, comm_recv_callback);
    //dal_cap_start();
}

//////////////////////////////////////////////////////////////////////////////////////////////
#include "upgrade.h"
#include "dal_delay.h"




static void comm_recv_init(void)
{
    //hw_init();
    //start_tasks();
    
    ecxxx_test();
    //fs_test();
    
}

void task_comm_recv_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    int cnt=0; 
    
    comm_recv_init();
    task_tmr_start(TASK_COMM_RECV, comm_tmr_callback, NULL, 200, TIME_INFINITE);
    
    while(1) {
        r = task_recv(TASK_COMM_RECV, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                                
                case EVT_COMM:
                {
                    err = comm_recv_proc(tasksHandle.hcom, e.arg, e.data, e.dLen);
                }
                break;
                
                case EVT_RS485:
                case EVT_LOG:
                {
                    //tmp_buf_t *tmp=&recvBuf.rs485;
                    //buf_t *tmp=&recvBuf.log;
                    //ecxxx_proc(tmp->buf[0]);
                    
                    //LOGD("____ recv: %s\n", recvBuf.rs485.buf);
                    //ecxxx_write(recvBuf.rs485.buf, recvBuf.rs485.dlen);
                    //tmp->dlen = 0;
                }
                break;
                
                case EVT_ECXXX:
                {
                    //rs485_write(recvBuf.ecxxx.buf, recvBuf.ecxxx.dlen);
                    //recvBuf.ecxxx.dlen = 0;
                }
                break;
                
                case EVT_TIMER:
                {
                    char tmp[50];
                    
                    sprintf((char*)tmp, "rs485 test %d\n", cnt++);
                    //dal_uart_write(rs485Handle, (u8*)tmp, strlen(tmp));
                }
                break;
            }
        }
    }
}

#endif


