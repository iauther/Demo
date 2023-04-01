#include "task.h"
#include "cap.h"
#include "log.h"
#include "list.h"


////////////////////////////////////////////
static void adc_data_callback(U8 *data, U32 len)
{
    node_t node={data, len};

    task_post(TASK_COMM, NULL, EVT_ADC, 0, &node, sizeof(node));
}
static void sai_data_callback(U8 *data, U32 len)
{
    node_t node={data, len};

    task_post(TASK_COMM, NULL, EVT_SAI, 0, &node, sizeof(node));
}
static void tmr_data_callback(U8 *data, U32 len)
{
    node_t node={data, len};
    
    //task_post(TASK_COMM, NULL, EVT_TMR, 0, &node, sizeof(node));
}



extern void list_cap_add(int id, void *data, int len);


#ifdef OS_KERNEL
static void start_task(int taskid, osPriority_t prio, int stkSize, osThreadFunc_t fn, int msgCnt, void *arg)
{
    task_attr_t attr={0};
    
    attr.func = fn,
    attr.arg = arg,
    attr.nMsg = msgCnt,
    attr.taskID = taskid,
    attr.priority = prio;//osPriorityNormal,
    attr.stkSize = stkSize,
    attr.runNow  = 1,
    task_new(&attr);
}

static void start_others(void)
{
    start_task(TASK_COMM,  osPriorityNormal, 1024, task_comm_fn,  5, NULL);
    //start_task(TASK_DATAPROC, osPriorityNormal, 1024, task_dataproc_fn, 5, NULL);
    //start_task(TASK_POLLING,  osPriorityNormal, 512,  task_polling_fn,  5, NULL);
}


static void cap_Init(void)
{
    cap_cfg_t cfg;
    
    cap_init();
    
    cfg.adc_fn = adc_data_callback;
    cfg.sai_fn = sai_data_callback;
    cfg.tmr_fn = tmr_data_callback;
    cap_config(&cfg);
}


void task_datacap_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    
    cap_Init();
    start_others();
    
    
    while(1) {
        r = task_recv(TASK_DATACAP, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                case EVT_ADC:
                {
                    node_t *nd=(node_t*)e.data;
                    
                    LOGD("___ EVT_ADC\n");
                    //list_cap_add(0, nd->ptr, nd->len);
                }
                break;
                
                case EVT_SAI:
                {
                    node_t *nd=(node_t*)e.data;
                    
                    LOGD("___ EVT_SAI\n");
                    //list_cap_add(0, nd->ptr, nd->len);
                }
                break;
                
                case EVT_TMR:
                {
                    node_t *nd=(node_t*)e.data;
                    //
                }
                break;
            }
            
        }
    }
}
#endif














