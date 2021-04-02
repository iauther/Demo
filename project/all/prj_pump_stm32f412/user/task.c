#include "task.h"
#include "drv/delay.h"
#include "rtx/rtx_lib.h" 


#ifdef OS_KERNEL

task_handle_t *task_handle[TASK_MAX];

handle_t task_create(int id, osThreadFunc_t task, U32 stack_size)
{
    task_handle_t *h;
    osThreadAttr_t attr = {
        .attr_bits  = 0U,
        .cb_mem     = NULL, //?
        .cb_size    = 0,
        .stack_mem  = NULL,
        .stack_size = stack_size,
        .priority   = osPriorityNormal,
        .tz_module  = 0,
    };
    
    h = calloc(1, sizeof(task_handle_t));
    if(!h) {
        return NULL;
    }
    
    h->thread_fn = task;
    h->msg = msg_init(5, sizeof(evt_t));
    h->thread_id = osThreadNew(h->thread_fn, h, (const osThreadAttr_t*)&attr);
    task_handle[id] = h;
    
    if(id!=TASK_MAIN) while(!h->running);
    
    return h;
}


static int msg_tx(int task_id, U8 evt, U8 type, void *data, U16 len, U8 bPost)
{
    int r;
    evt_t e;
    task_handle_t *h=task_handle[task_id];
    
    if(!h || len>MAXLEN) {
        return -1;
    }
    
    e.evt = evt;
    e.type = type;
    e.dLen = len;
    if(data && len>0) {
        memcpy(e.data, data, len);
    }
    
    if(bPost) {
        r = msg_post(h->msg, &e, sizeof(e));
    }
    else {
        r = msg_send(h->msg, &e, sizeof(e));
    }
    
    return r;
}
int task_msg_send(int task_id, U8 evt, U8 type, void *data, U16 len)
{
    return msg_tx(task_id, evt, type, data, len, 0);
}


int task_msg_post(int task_id, U8 evt, U8 type, void *data, U16 len)
{
    return msg_tx(task_id, evt, type, data, len, 1);
}



static void task_main(void *arg)
{
    task_create(TASK_COM,  task_com_fn,  1024);
    task_create(TASK_DEV,  task_dev_fn,  1024);
    task_create(TASK_MISC, task_misc_fn, 1024);
    
    while(1){
        delay_ms(500);
    }
}



int task_start(void)
{
    osKernelInitialize();                 // Initialize CMSIS-RTOS
    task_create(TASK_MAIN,  task_main, 256);
    osKernelStart();
    
    return 0;
}


#endif





