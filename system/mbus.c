#include "mbus.h"
#include "mb.h"
#include "task.h"

static U8 mbusQuit=0;
static void mbus_task(void *arg)
{
    while(1) {
        eMBPoll();
        if(mbusQuit) {
            break;
        }
    }
}
static void mbus_task_start(void *arg)
{
#ifdef OS_KERNEL
    osThreadAttr_t  attr={0};
    
    mbusQuit = 0;
    attr.name = "mbus";
    attr.stack_size = 1024;
    attr.priority = osPriorityNormal;
    osThreadNew(mbus_task, arg, &attr);
#endif
}
static void mbus_task_stop(void)
{
    mbusQuit = 1;
}


int mbus_init(void)
{
    eMBInit(MB_RTU, 0x01, 0, 9600, MB_PAR_NONE);    // 初始化modbus为RTU方式，波特率9600，无校验
    
    //eMBSetSlaveID(0xab);
    eMBEnable();
    mbus_task_start(NULL);
    
    return 0;
}


int mbus_poll(void)
{
    return eMBPoll();
}









