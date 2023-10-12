#include "task.h"


#ifdef OS_KERNEL

task_handle_t *taskHandle[TASK_MAX]={NULL};
static int get_free_id(void)
{
    int i;
    for(i=0; i<TASK_MAX; i++) {
        if(taskHandle[i]==NULL) {
            return i;
        }
    }
    
    return -1;
}


static int msg_tx(task_handle_t *h, void *addr, U8 evt, U8 type, void *data, U16 len, U32 timeout)
{
    int r;
    evt_t e;
    
    if(!h || len>EVT_DATA_LEN_MAX) {
        return -1;
    }
    
    e.arg = addr;
    e.evt = evt;
    e.type = type;
    e.dLen = len;
    if(data && len>0) {
        memcpy(e.data, data, len);
    }
    
    if(timeout) {
        r = msg_send(h->msg, &e, sizeof(e), timeout);
    }
    else {
        r = msg_post(h->msg, &e, sizeof(e));
    }
    
    return r;
}

int task_wait(int taskID)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    if(osKernelGetState()==osKernelRunning) {
        //osThreadFlagsWait();
        while(osThreadGetState(h->threadID)!=osThreadBlocked);
    }
    
    return 0;
}

static void task_free(task_handle_t **h)
{
    taskHandle[(*h)->attr.taskID] = NULL;
    free(*h); 
}

/////////////////////////////////////////////////////////////////////////
static int create_user_task(void)
{
    task_attr_t att={
        .func = task_comm_recv_fn,
        .arg = NULL,
        .nMsg = 5,
        .taskID = TASK_COMM_RECV,
        .priority = osPriorityNormal,
        .stkSize = 2048,
        .runNow  = 1,
    };
    task_new(&att);
    
    return 0;
}
void task_init(void)
{
    osKernelInitialize();
    create_user_task();
    osKernelStart();
}



int task_new(task_attr_t *tattr)
{
    task_handle_t *h=NULL;
    
    if(!tattr || tattr->taskID>=TASK_MAX) {
        return -1;
    }
    
    const osThreadAttr_t attr={
        .attr_bits  = 0U,
        .cb_mem     = NULL, //?
        .cb_size    = 0,
        .stack_mem  = NULL,
        .stack_size = tattr->stkSize,
        .priority   = tattr->priority,
        .tz_module  = 0,
    };
    
    h = calloc(1, sizeof(task_handle_t));
    if(!h) {
        return -1;
    }
    
    h->attr = *tattr;
    if(h->attr.nMsg>0) {
        h->msg = msg_init(h->attr.nMsg, sizeof(evt_t));
    }
    
    h->threadID = osThreadNew(h->attr.func, h->attr.arg, &attr);    
    
    taskHandle[h->attr.taskID] = h;
    
    if(osKernelGetState()==osKernelRunning) {
        if(h->attr.runNow) {
            osThreadYield();
            while(osThreadGetState(h->threadID)!=osThreadBlocked);
        }
        else {
            osThreadSuspend(h->threadID);
        }
    }
    
    return 0;
}


int start_task_simp(osThreadFunc_t fn, int stksize, void *arg, osThreadId_t *tid)
{
    osThreadId_t id;
    const osThreadAttr_t attr={
        .attr_bits  = 0U,
        .cb_mem     = NULL, //?
        .cb_size    = 0,
        .stack_mem  = NULL,
        .stack_size = stksize,
        .priority   = osPriorityNormal,
        .tz_module  = 0,
    };
    
    id = osThreadNew(fn, arg, &attr); 
    if(tid) *tid = id;
    
    return 0;
}




int task_start(int taskID)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    return osThreadResume(h->threadID);
}


int task_stop(int taskID)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    return osThreadSuspend(h->threadID);
}


int task_quit(int taskID)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    osThreadTerminate(h->threadID);
    task_free(&h);
    
    return 0;
}


int task_recv(int taskID, evt_t *evt, int evtlen)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    return msg_recv(h->msg, evt, evtlen);
}


int task_send(int taskID, void *addr, U8 evt, U8 type, void *data, U16 len, U32 timeout)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    return msg_tx(h, addr, evt, type, data, len, timeout);
}


int task_post(int taskID, void *addr, U8 evt, U8 type, void *data, U16 len)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    return msg_tx(h, addr, evt, type, data, len, 0);
}


int task_trig(int taskID, U8 evt)
{
    return task_post(taskID, NULL, evt, 0, NULL, 0);
}


int task_msg_clear(int taskID)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    return msg_reset(h->msg);
}


static void timer_callback_fn(void *arg)
{
    timer_handle_t *h=(timer_handle_t*)arg;
    
    if(h && h->fn) {
        if(h->tTimes>0) {
            if(h->cTimes<h->tTimes) {
                h->fn(h->arg);
                h->cTimes++;
            }
            else {
                osTimerStop(h->tid);
            }
        }
        else {
            h->fn(h->arg);
        }
    }
}
handle_t task_timer_init(osTimerFunc_t fn, void *arg, int ms, int times)
{
    osTimerType_t type;
    osStatus_t st;
    timer_handle_t *h=malloc(sizeof(task_handle_t));
    
    if(!h || !fn || !ms || !times) {
        return NULL;
    }
    
    h->tTimes = times;
    h->cTimes = 0;
    h->fn = fn;
    h->arg = arg;
    h->ms  = ms;
    
    type = (times==1)?osTimerOnce:osTimerPeriodic;
    h->tid = osTimerNew(timer_callback_fn, type, h, NULL);
    if(!h->tid) {
        free(h);
        return NULL;
    }
    
    return h;
}


int task_timer_start(handle_t h)
{
    osStatus_t st;
    timer_handle_t *th=(timer_handle_t*)h;
    
    if(!th) {
        return -1;
    }
    
    st = osTimerStart(th->tid, th->ms);
    if(st!=osOK) {
        return -1;
    }
    
    return 0;
}


int task_timer_stop(handle_t h)
{
    timer_handle_t *th=(timer_handle_t*)h;
    
    if(!th) {
        return -1;
    }
    
    osTimerStop(th->tid);
    
    
    return 0;
}

int task_timer_free(handle_t h)
{
    timer_handle_t *th=(timer_handle_t*)h;
    
    if(!th) {
        return -1;
    }
    
    osTimerDelete(th->tid);
    free(th);
    
    return 0;
}


int task_yield(void)
{
    return osThreadYield();
}


#endif





