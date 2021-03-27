#include "task.h"                          // CMSIS RTOS header file
 
typedef struct {
    
    msg_t           msg;
    int             priority;
    osThreadId_t    thread_id;
    osThreadFunc_t  thread_fn;
}task_handle_t;


task_handle_t *task_handle[TASK_MAX];


//osThreadId_t osThreadNew (osThreadFunc_t func, void *argument, const osThreadAttr_t *attr);
handle_t task_create(osThreadFunc_t task, int priority)
{
    return osThreadNew(task, NULL, NULL);
}


int task_start(void)
{
    //task_create();
    //
    
    return 0;
}



int task_init(void)
{
    osKernelInitialize();                 // Initialize CMSIS-RTOS
    task_start();
    
    return 0;
}


void task_loop(void)
{
    osKernelStart();                      // Start thread execution
}






