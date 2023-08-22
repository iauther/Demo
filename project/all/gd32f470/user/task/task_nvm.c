#include "task.h"
#include "list.h"
#include "dal_delay.h"


#ifdef OS_KERNEL

typedef struct {
    int     id;
    void    *data;
    int     dlen;
}file_node_t;



static handle_t fList=NULL;
void nvm_trigger(int id, void *data, int len, int trig)
{
    list_append(fList, data, len);
}


void task_nvm_fn(void *arg)
{
    int i,r;
    U8  err;
    node_t node;
    list_cfg_t cfg;
    
    LOGD("_____ task nvm running\n");
    
    cfg.mode = MODE_FILO;
    cfg.max = 5;
    fList = list_init(&cfg);
    
    while(1) {
        while(list_get(fList, &node, 0)==0) {
            
            //paras_write(e.type, node.buf, node.dlen);
            list_remove(fList, 0);
        }
        
        osDelay(1);
    }
    
}
#endif


