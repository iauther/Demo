#include "task.h"
#include "cap.h"
#include "log.h"
#include "list.h"



extern void list_cap_add(int id, void *data, int len);
////////////////////////////////////////////
static void adc_data_callback(U8 *data, U32 len)
{
    node_t node={data, len};

    //task_post(TASK_DATACAP, NULL, EVT_ADC, 0, &node, sizeof(node));
}
static void sai_data_callback(U8 *data, U32 len)
{
    node_t node={data, len};

    //task_post(TASK_DATACAP, NULL, EVT_SAI, 0, &node, sizeof(node));
}
static void tmr_data_callback(U8 *data, U32 len)
{
    node_t node={data, len};
    
    //task_post(TASK_DATACAP, NULL, EVT_TMR, 0, &node, sizeof(node));
}

void cap_Init(void)
{
    cap_cfg_t cfg;
    
    cap_init();
    
    cfg.adc_fn = adc_data_callback;
    cfg.sai_fn = sai_data_callback;
    cfg.tmr_fn = tmr_data_callback;
    cap_config(&cfg);
}


#ifdef OS_KERNEL
void task_datacap_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;    
    
    cap_start();
    while(1) {
        r = task_recv(TASK_DATACAP, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                case EVT_ADC:
                {
                    node_t *nd=(node_t*)e.data;
                    
                    //LOGD("___ EVT_ADC\n");
                    list_cap_add(EVT_DATA_PROC, nd->ptr, nd->len);
                    task_post(TASK_DATAPROC, NULL, EVT_DATA_PROC, 0, NULL, 0);
                }
                break;
                
                case EVT_SAI:
                {
                    node_t *nd=(node_t*)e.data;
                    
                    //LOGD("___ EVT_SAI\n");
                    list_cap_add(EVT_DATA_STAT, nd->ptr, nd->len);
                    task_post(TASK_DATAPROC, NULL, EVT_DATA_STAT, 0, NULL, 0);
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














