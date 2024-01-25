#ifndef __TASK_Hx__
#define __TASK_Hx__

#include "protocol.h"
#include "dal_adc.h"
#include "log.h"
#include "evt.h"
#include "rbuf.h"

#ifdef OS_KERNEL
#include "cmsis_os2.h"
#include "msg.h"
#include "rbuf.h"


#define TIME_INFINITE  0xffffffff

enum {
    TASK_COMM_RECV=0,
    TASK_COMM_SEND,
    TASK_DATA_CAP,
    TASK_DATA_PROC,
    TASK_POLLING,
    TASK_SEND,
    
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
    osThreadId_t    threadID;
}task_handle_t;


typedef struct {
    osTimerId_t     tid;
    void            *arg;
    osTimerFunc_t   fn;
    int             tTimes;
    int             cTimes;
    int             ms;
}timer_handle_t;

////////////////////////////////

typedef struct {
    U8              ch;
    U64             time;
    U32             cnt;
    F32             data[0];
}raw_data_t;

typedef struct {
    int             pdt;
    int             hdt;
    int             hlt;
    int             mdt;
}index_t;

typedef struct {
    F32             *data;
    int             dcnt;
    int             bcnt;
}threshold_buf_t;

typedef struct {
    threshold_buf_t pre;
    threshold_buf_t bdy;
    threshold_buf_t post;
    F32             vth;
    F32             max;
    U64             time;
    int             idx;
    index_t         cur;
    index_t         set;
    int             trigged;
}threshold_t;

typedef struct {
    buf_t           cap;            //采集数据用临时内存
    buf_t           prc;            //转换数据用临时内存
    
    int             rlen;           //recved len
    int             slen;           //skiped len
    int             times;          //
    
    handle_t        raw;            //原始采集数据列表
    threshold_t     thr;            //阈值数据缓存列表
    
    
    handle_t        rb;
}ch_var_t;

typedef struct {
    ch_var_t        var[CH_MAX];
    
    handle_t        file;            //文件存储列表
    handle_t        send;            //发送数据列表
}task_buf_t;

typedef struct {
    handle_t        hcomm;
    
    handle_t        hMem;
    
    U8              chs;
    U8              chBits;
    
    handle_t        rb;              //rbuf
}tasks_handle_t;

///////////////////////////////////////////////////


extern task_buf_t taskBuffer;
extern tasks_handle_t tasksHandle;


void task_comm_recv_fn(void *arg);
void task_comm_send_fn(void *arg);
void task_data_cap_fn(void *arg);
void task_data_proc_fn(void *arg);
void task_polling_fn(void *arg);
void task_mqtt_fn(void *arg);
void task_send_fn(void *arg);


int api_cap_start(U8 ch);
int api_cap_stop(U8 ch);

int api_cap_start_all(void);
int api_cap_stop_all(void);

int api_cap_power(U8 ch, U8 on);

int api_comm_connect(U8 port);
int api_comm_disconnect(void);
int api_comm_send_para(void);
int api_comm_is_connected(void);

int api_comm_send_ack(U8 type, U8 err);
int api_comm_send_data(U8 type, U8 nAck, void *data, int len);
int api_send_append(void *data, int len);
int api_send_save_file(void);
int api_send_is_finished(void);

int api_data_proc_send(U8 ch, void *data, int len);


void task_init(void);
int task_new(task_attr_t *atrr);
int start_task_simp(osThreadFunc_t fn, int stksize, void *arg, osThreadId_t *tid);
int task_start(int taskID);
int task_stop(int taskID);
int task_quit(int id);
int task_wait(int taskID);

int task_recv(int taskID, evt_t *evt, int evtlen);
int task_send(int taskID, void *arg, U8 evt, U8 type, void *data, int len, U32 timeout);
int task_post(int taskID, void *arg, U8 evt, U8 type, void *data, int len);
int task_trig(int taskID, U8 evt);
int task_trig2(int taskID, U8 evt, U8 data);
int task_msg_clear(int taskID);

handle_t task_timer_init(osTimerFunc_t fn, void *arg, int ms, int times);
int task_timer_start(handle_t h);
int task_timer_stop(handle_t h);
int task_timer_free(handle_t h);

int task_yield(void);
U32 task_stack_remain(void);
void test_main(void);
void print_ts(char *s);

#endif

void test_main(void);



#endif

