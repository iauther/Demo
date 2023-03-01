#ifndef __TASK_Hx__
#define __TASK_Hx__

#ifdef OS_KERNEL
#include "cmsis_os2.h"
#include "msg.h"


#define TASK_MAX   5

typedef struct {
    osThreadFunc_t  func;
    void            *arg;
    int             nMsg;       //msg max count
    int             prio;       //priority
    int             stkSize;    //stack size
    U8              runNow;
}task_attr_t;

typedef struct {
    msg_t           *msg;
    
    task_attr_t     attr;
    
    osTimerId_t     tmrID;
    osTimerFunc_t   tmrFunc;
    U32             allTimes;
    U32             curTimes;        
    
    osThreadId_t    threadID;
    osThreadFunc_t  threadFun;
}task_handle_t;


void task_com_fn(void *arg);
void task_dev_fn(void *arg);
void task_misc_fn(void *arg);

void task_init(void);

int task_new(task_attr_t *atrr);
int task_start(int taskID);
int task_stop(int taskID);
int task_quit(int id);

int task_recv(int taskID, evt_t *evt, int evtlen);
int task_send(int taskID, U8 evt, U8 type, void *data, U16 len, U32 timeout);
int task_post(int taskID, U8 evt, U8 type, void *data, U16 len);

int task_tmr_start(int taskID, osTimerFunc_t tmrFunc, U32 ms, U32 times);
int task_tmr_stop(int taskID);
#endif

void test_main(void);



#endif

