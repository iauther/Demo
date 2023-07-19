#include "net.h"
#include "comm.h"
#include "log.h"
#include "lock.h"
#include "dal.h"
#include "task.h"
#include "fs.h"
#include "paras.h"
#include "json.h"
#include "xmem.h"
#include "dal_cap.h"

tasks_handle_t tasksHandle={0};
U64 sdramBuffer[XMEM_LEN/8] __attribute__ ((at(XMEM_ADDR)));



static void task_mem_init(void)
{
    list_cfg_t cfg;
    
    tasksHandle.hMem = xmem_init(sdramBuffer, XMEM_LEN);
    
    tasksHandle.capBuf.blen = ADC_SAMPLE_POINTS*4*2+32;
    tasksHandle.capBuf.buf = malloc(tasksHandle.capBuf.blen); 
    
    tasksHandle.calBuf.blen = ADC_SAMPLE_POINTS*4*2+32;
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
    
    lc.max = 10;
    tasksHandle.connList = list_init(&lc);
}


static int conn_iterator_fn(handle_t l, node_t *node, node_t *xd, void *arg, int *act)
{
    int *idx=(int*)arg;
    
    if(node->dlen==xd->dlen) {
        if(memcmp(node->buf, xd->buf, node->dlen)==0) {
            *act = ACT_STOP;
            return 0;
        }
    }
    (*idx)++;
    
    *act = ACT_NONE;
    return -1;
}
static int conn_find(handle_t list, void *conn)
{
    int r,index;
    node_t node={&conn, sizeof(conn), sizeof(conn)};
    
    r = list_iterator(list, &node, conn_iterator_fn, &index);
    if(!r) {
        index = -1;
    }
        
    return index;
}


static int comm_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int r=-1;
    
    switch(evt) {
        case NET_EVT_NEW_CONN:
        list_append(tasksHandle.connList, &addr, sizeof(addr));
        break;
        
        case NET_EVT_DIS_CONN:
        {
            int idx = conn_find(tasksHandle.connList, addr);
            if(idx>=0) {
                list_remove(tasksHandle.connList, idx);
            }
        }
        break;
        
        case NET_EVT_DATA_IN:
        r = task_post(TASK_COMM_RECV, addr, EVT_COMM, evt, data, len);
        break;
    }
    
    return r;
}

static int json_parse(char *json)
{
    int len,f=0;
    char *js=json;
    char tmp[2]={0};
    
    cfg_clear();
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
    
    json_to_ch(js, tasksHandle.cfg, CH_MAX);
    
    if(f) free(js);
    
    return 0;
}

extern void cap_config(void);
static int hw_init(void)
{
    int r=0;
    
    lock_staic_init();
    
    log_init();
    fs_init();
    paras_load();
    
    json_init();
    json_parse(NULL);
    
    dal_cap_init();
    cap_config();
    
    task_mem_init();
    task_list_cfg();
    
    return r;
}



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
    start_task(TASK_COMM_SEND,  osPriorityNormal, 1024,  task_comm_send_fn,  5, NULL);
    start_task(TASK_DATA_CAP,   osPriorityNormal, 1024, task_data_cap_fn,  5, NULL);
    start_task(TASK_DATA_PROC,  osPriorityNormal, 1024, task_data_proc_fn, 5, NULL);
    //start_task(TASK_POLLING,  osPriorityNormal, 512,  task_polling_fn,  5, NULL);
    
    task_wait(TASK_DATA_PROC);
    LOGD("______ task wait finished\n");
    
    tasksHandle.hcom = comm_init(PORT_NET, comm_recv_callback);
    dal_cap_start();
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
    
    comm_recv_init();
    while(1) {
        r = task_recv(TASK_COMM_RECV, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                                
                case EVT_COMM:
                {
                    err = comm_recv_proc(tasksHandle.hcom, e.arg, e.data, e.dLen);
                }
                break;
            }
        }
    }
}
#endif


