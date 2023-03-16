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
    
    e.addr = addr;
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

static int task_wait(int taskID)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    if(osThreadGetState(h->threadID) != osThreadBlocked) {
        osThreadYield();
    }
    
    return 0;
}

static void tmr_callback(void *arg)
{
    task_handle_t *h=(task_handle_t*)arg;
    if(h && h->tmrFunc && h->allTimes>0 && h->curTimes<h->allTimes) {
        h->tmrFunc(arg);
        h->curTimes++;
    }
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
        .func = task_com_fn,
        .arg = NULL,
        .nMsg = 5,
        .taskID = TASK_COM,
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
    
    if(!h->attr.runNow) {
        osThreadSuspend(h->threadID);
    }
    
    taskHandle[h->attr.taskID] = h;
    
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



int task_tmr_start(int taskID, osTimerFunc_t tmrFunc, void *arg, U32 ms, U32 times)
{
    osTimerType_t type;
    osStatus_t st;
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    h->allTimes = times;
    h->curTimes = 0;
    h->tmrFunc = tmrFunc;
    h->tmrArg = arg;
    
    type = (times>1)?osTimerPeriodic:osTimerOnce;
    //if(h->tmrID) osTimerDelete(h->tmrID);
    
    h->tmrID = osTimerNew(tmr_callback, type, h, NULL);
    if(!h->tmrID) {
        return -1;
    }
    
    st = osTimerStart(h->tmrID, ms);
    
    return st;
}


int task_tmr_stop(int taskID)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    osTimerStop(h->tmrID);
    osTimerDelete(h->tmrID);
    h->tmrID = NULL;
    
    return 0;
}

#endif





