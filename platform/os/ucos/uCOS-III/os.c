#ifdef UCOSIII

#include "includes.h"
#include "cmsis_os2.h"


#define STK_SIZE    512
CPU_STK taskStk[STK_SIZE];


//OS_CPU_SR  cpu_sr=0;





osStatus_t osKernelInitialize (void)
{
    OS_ERR err;
    OSInit(&err);
    CPU_Init();           //????

    return osOK;
}
osStatus_t osKernelStart (void)
{
    OS_ERR err;
    OSStart(&err);

    return osOK;
}

OS_TCB taskTCB;
typedef struct {
    OS_TCB   tcb;
    CPU_STK  *stk;

    OS_TMR    tmr;
    CPU_STK_SIZE stk_size;
}task_t;
osThreadId_t osThreadNew (osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)
{
    OS_ERR err;
    #if 0
        typedef struct {
      const char                   *name;   ///< name of the thread
      uint32_t                 attr_bits;   ///< attribute bits
      void                      *cb_mem;    ///< memory for control block
      uint32_t                   cb_size;   ///< size of provided memory for control block
      void                   *stack_mem;    ///< memory for stack
      uint32_t                stack_size;   ///< size of stack
      osPriority_t              priority;   ///< initial thread priority (default: osPriorityNormal)
      TZ_ModuleId_t            tz_module;   ///< TrustZone module identifier
      uint32_t                  reserved;   ///< reserved (must be 0)
    } osThreadAttr_t;
    #endif

    //OS_CRITICAL_ENTER();
    OSTaskCreate((OS_TCB 	* )&taskTCB,		    //任务控制块
				 (CPU_CHAR	* )"task", 		        //任务名字
                 (OS_TASK_PTR )func, 			    //任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )8,                   //任务优先级
                 (CPU_STK   * )&taskStk[0],	        //任务堆栈基地址
                 (CPU_STK_SIZE)STK_SIZE/10,	        //任务堆栈深度限位
                 (CPU_STK_SIZE)STK_SIZE,		    //任务堆栈大小
                 (OS_MSG_QTY  )10,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值

    //OS_CRITICAL_EXIT();
    
    return 0;
}
osStatus_t osDelay (uint32_t ticks)
{
    OS_ERR err; 
	OSTimeDly(ticks, OS_OPT_TIME_PERIODIC, &err);
    return osOK;
}

typedef struct {
    OS_TICK   time;
    OS_TMR    tmr;
}tmr_t;
osTimerId_t osTimerNew (osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr)
{
    OS_ERR err; 
    OS_TMR tmr;

    OS_OPT opt=(type==osTimerPeriodic)?OS_OPT_TMR_PERIODIC:OS_OPT_TMR_ONE_SHOT;
    OSTmrCreate(&tmr, "tmr", 0, 50, opt, (OS_TMR_CALLBACK_PTR)func, argument, &err);
    
    return 0;
}
osStatus_t osTimerStart (osTimerId_t timer_id, uint32_t ticks)
{
    OS_ERR err; 
    OSTmrStart((OS_TMR*)timer_id, &err);
    return osOK;
}
osStatus_t osTimerStop (osTimerId_t timer_id)
{
    OS_ERR err;

    OSTmrStop((OS_TMR*)timer_id, OS_OPT_TMR_NONE, 0, &err);

    return osOK;
}
/////////////////////////////////////


OS_Q msgQ;
OS_MSG_SIZE msgSz;
typedef struct {
    OS_Q         q;
    OS_MSG_SIZE  sz;
}msgq_t;


osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)
{
    OS_ERR err; 

    msgSz = msg_size;
    OSQCreate(&msgQ, "", msg_count, &err);
    
    return (osMessageQueueId_t)&msgQ;
}
osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout)
{
    OS_ERR err; 

    OSQPost((OS_Q*)mq_id, (void*)msg_ptr, msgSz, OS_OPT_POST_FIFO, &err);
    
    return osOK;
}
osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout)
{
    u8 *p;
    OS_ERR err;
    OS_MSG_SIZE sz;

    u32 time=(timeout==osWaitForever)?0:timeout;

    p = OSQPend((OS_Q*)mq_id, time, OS_OPT_PEND_BLOCKING, &sz, NULL, &err);
    if(p) {
        memcpy(msg_ptr, p, sz);
    }
    
    return osOK;
}


#endif

