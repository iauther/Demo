#include "task.h"
#include "dal_cap.h"
#include "log.h"
#include "list.h"
#include "cfg.h"
#include "json.h"
#include "fs.h"



////////////////////////////////////////////
static void adc_data_callback(U8 *data, U32 len)
{
    node_t node={data, len};

    task_post(TASK_DATA_CAP, NULL, EVT_ADC, 0, &node, sizeof(node));
}
static void sai_data_callback(U8 *data, U32 len)
{
    node_t node={data, len};

    //task_post(TASK_DATA_CAP, NULL, EVT_SAI, 0, &node, sizeof(node));
}
static void tmr_data_callback(U8 *data, U32 len)
{
    node_t node={data, len};
    
    //task_post(TASK_DATA_CAP, NULL, EVT_TMR, 0, &node, sizeof(node));
}

static int json_parse(char *json)
{
    int jlen,f=0;
    char *js=json;
    
    if(!js) {
        jlen = fs_read(FILE_CH, NULL, 0);
        if(jlen<=0) {
            return -1;
        }
        js = malloc(jlen);
        if(!js) {
            return -1;
        }
        
        fs_read(FILE_CH, js, jlen);
        
        f = 1;
    }
    json_to_ch(js, tasksHandle.cfg, CH_MAX);
    
    if(f) free(js);
    
    return 0;
}




void cap_config(void)
{
    dal_cap_cfg_t cfg;
    
    json_parse(NULL);
    
    cfg.adc_fn = adc_data_callback;
    cfg.sai_fn = sai_data_callback;
    cfg.tmr_fn = tmr_data_callback;
    dal_cap_config(&cfg);
    
    dal_cap_start();
}


#ifdef OS_KERNEL
extern void adc_data_fill(U8 dual, U32 *buf, int len);
extern void sai_data_fill(U8 chBits, U32 *buf, int len);
void task_data_cap_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;    
    
    cap_config();
    while(1) {
        r = task_recv(TASK_DATA_CAP, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                case EVT_ADC:
                {
                    node_t *nd=(node_t*)e.data;
                    
                    //LOGD("___ EVT_ADC\n");
                    adc_data_fill(ADC_DUAL_MODE, nd->ptr, nd->len);
                    r = task_post(TASK_DATA_PROC, NULL, EVT_DATA, 0, NULL, 0);
                }
                break;
                
                case EVT_SAI:
                {
                    node_t *nd=(node_t*)e.data;
                    
                    //LOGD("___ EVT_SAI\n");
                    sai_data_fill(0xff, nd->ptr, nd->len);
                    task_post(TASK_DATA_PROC, NULL, EVT_DATA, 0, NULL, 0);
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














