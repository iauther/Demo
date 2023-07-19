#include "task.h"
#include "list.h"
#include "paras.h"


#define POLL_MS    100
#ifdef OS_KERNEL

typedef struct {
    int     id;
    void    *data;
    int     len;
}file_node_t;


static U32 tmrCount=0;
static handle_t fList=NULL;
static void polling_tmr_callback(void *arg)
{
    task_post(TASK_POLLING, NULL, EVT_TIMER, 0, NULL, 0);
}

void save_file_trigger(int id, void *data, int len, int trig)
{
    list_append(fList, data, len);
    if(trig>0) {
        task_post(TASK_POLLING, NULL, EVT_SAVE, id, NULL, 0);
    }
}


void task_polling_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    osTimerId_t tmrId;
    list_cfg_t cfg;
    task_handle_t *h=(task_handle_t*)arg;
    
    LOGD("_____ task_polling running\n");
    
    cfg.mode = MODE_FILO;
    cfg.max = 10;
    cfg.hmem = NULL;
    fList = list_init(&cfg);
    task_tmr_start(TASK_POLLING, polling_tmr_callback, NULL, POLL_MS, TIME_INFINITE);
    while(1) {
        r = task_recv(TASK_POLLING, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_TIMER:
                {
                    tmrCount++;
                    //eth_link_check();
                    
                }
                break;
                
                case EVT_SAVE:
                {
                    node_t node;
                    
                    while(1) {
                        r = list_get(fList, &node, 0);
                        if(r) {
                            break;
                        }
                        
                        paras_write(e.type, node.buf, node.dlen);
                        list_remove(fList, 0);
                    }
                }
                break;
                
                default:
                continue;
            }
        }
    }
}
#endif


