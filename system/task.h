#ifndef __TASK_Hx__
#define __TASK_Hx__

#ifdef OS_KERNEL
#include "cmsis_os2.h"
#include "msg.h"


#define TIME_INFINITE  0xffffffff

enum {
    TASK_COMM_RECV=0,
    TASK_COMM_SEND,
    TASK_DATA_CAP,
    TASK_DATA_PROC,
    TASK_POLLING,
    
    TASK_MAX
};

typedef struct {
    osThreadFunc_t  func;
    void            *arg;
    int             nMsg;       //msg max count
    int             taskID;
    osPriority_t    priority;
    int             stkSize;    //stack size
    int             runNow;
}task_attr_t;

typedef struct {
    msg_t           *msg;
    
    task_attr_t     attr;
    
    osTimerId_t     tmrID;
    void            *tmrArg;
    osTimerFunc_t   tmrFunc;
    U32             allTimes;
    U32             curTimes;        
    
    osThreadId_t    threadID;
}task_handle_t;


typedef struct {
    handle_t    hcom;
    void        *netAddr;
}comm_handle_t;


extern handle_t      hMem;
extern comm_handle_t commHandle;


void task_comm_recv_fn(void *arg);
void task_comm_send_fn(void *arg);
void task_data_cap_fn(void *arg);
void task_data_proc_fn(void *arg);
void task_polling_fn(void *arg);

void task_init(void);
int task_new(task_attr_t *atrr);
int task_start(int taskID);
int task_stop(int taskID);
int task_quit(int id);

int task_recv(int taskID, evt_t *evt, int evtlen);
int task_send(int taskID, void *addr, U8 evt, U8 type, void *data, U16 len, U32 timeout);
int task_post(int taskID, void *addr, U8 evt, U8 type, void *data, U16 len);

int task_tmr_start(int taskID, osTimerFunc_t tmrFunc, void *arg, U32 ms, U32 times);
int task_tmr_stop(int taskID);
#endif

void test_main(void);



#endif

