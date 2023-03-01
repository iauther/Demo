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


static int msg_tx(task_handle_t *h, U8 evt, U8 type, void *data, U16 len, U32 timeout)
{
    int r;
    evt_t e;
    
    if(!h || len>MAXLEN) {
        return -1;
    }
    
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

/////////////////////////////////////////////////////////////////////////
static void create_user_task(void)
{
    task_attr_t att={
        .func = task_com_fn,
        .arg = NULL,
        .nMsg = 5,
        .prio = osPriorityNormal,
        .stkSize = 1024,
        .runNow  = 1,
    };
    
    task_new(&att);
}
void task_init(void)
{
    osKernelInitialize();
    create_user_task();
    osKernelStart();
}



int task_new(task_attr_t *tattr)
{
    int id=get_free_id();
    task_handle_t *h=NULL;
    
    if(!tattr || id<0) {
        return -1;
    }
    
    const osThreadAttr_t attr={
        .attr_bits  = 0U,
        .cb_mem     = NULL, //?
        .cb_size    = 0,
        .stack_mem  = NULL,
        .stack_size = tattr->stkSize,
        .priority   = tattr->prio,
        .tz_module  = 0,
    };
    
    h = calloc(1, sizeof(task_handle_t));
    if(!h) {
        return NULL;
    }
    
    h->attr = *tattr;
    h->msg = msg_init(h->attr.nMsg, sizeof(evt_t));
    h->threadID = osThreadNew(h->attr.func, h, &attr);
    
    taskHandle[id] = h;
    
    if(h->attr.runNow==0) {
        osThreadSuspend(h->threadID);
    }
    
    return id;
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
    
    return osThreadTerminate(h->threadID);
}


int task_recv(int taskID, evt_t *evt, int evtlen)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    return msg_recv(h->msg, evt, sizeof(evt_t));
}


int task_send(int taskID, U8 evt, U8 type, void *data, U16 len, U32 timeout)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    return msg_tx(h, evt, type, data, len, timeout);
}


int task_post(int taskID, U8 evt, U8 type, void *data, U16 len)
{
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    return msg_tx(h, evt, type, data, len, 1);
}



int task_tmr_start(int taskID, osTimerFunc_t tmrFunc, U32 ms, U32 times)
{
    osTimerType_t type;
    task_handle_t *h=taskHandle[taskID];
    
    if(taskID<0 || taskID>=TASK_MAX || !h || !h->threadID) {
        return -1;
    }
    
    h->allTimes = times;
    h->curTimes = 0;
    
    type = (times<=0)?osTimerPeriodic:osTimerOnce;
    osTimerDelete(h->tmrID);
    h->tmrID = osTimerNew(tmr_callback, type, h, NULL);
    if(!h->tmrID) {
        return -1;
    }
    
    return osTimerStart(h->tmrID, ms);
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





