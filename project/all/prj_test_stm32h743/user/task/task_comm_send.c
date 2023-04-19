#include "com.h"
#include "log.h"
#include "cap.h"
#include "task.h"
#include "list.h"


typedef struct {
    handle_t        list;     //原始数据
}comm_send_handle_t;



static comm_send_handle_t csHandle;

#ifdef OS_KERNEL
static void comm_send_init(int max)
{
    list_cfg_t cfg;
    
    cfg.max = max;
    cfg.mode = MODE_FIFO;
    cfg.hmem = hMem;
    
    csHandle.list = list_new(&cfg);
}


void comm_send_trigger(void *addr)
{
    task_post(TASK_COMM_RECV, addr, EVT_COMM, 0, NULL, 0);
}

void task_comm_send_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    node_t node;
    
    comm_send_init(10);
    while(1) {
        r = task_recv(TASK_COMM_SEND, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                                
                case EVT_COMM:
                {
                    while(1) {
                        r = list_get(csHandle.list, &node, 0);
                        if(r) {
                            break;
                        }
                        
                        err = com_send_data(commHandle.hcom, e.arg, TYPE_DATA, 0, node.ptr, node.len);
                        if(err==0) {
                            list_remove(csHandle.list, 0);
                        }
                    }
                }
                break;
            }
        }
    }
}
#endif


