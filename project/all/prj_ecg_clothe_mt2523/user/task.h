#ifndef __TASK_Hx__
#define __TASK_Hx__

#include "RTE_Components.h"
#include "cmsis_os2.h"
#include "msg.h"

enum {
    TASK_COM=0,
    TASK_DEV,
    TASK_CHK,
    
    TASK_MAX
};



void task_com_fn(void *argument);
void task_dev_fn(void *argument);
void task_chk_fn(void *argument);

handle_t task_create(osThreadFunc_t task, int priority);
int task_start(void);

int task_init(void);
void task_loop(void);

#endif

