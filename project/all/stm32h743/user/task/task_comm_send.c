#include "comm.h"
#include "task.h"
#include "list.h"
#include "dal_delay.h"



#ifdef OS_KERNEL
void comm_send_trigger(void)
{
    task_post(TASK_COMM_SEND, NULL, EVT_COMM, 0, NULL, 0);
}

static int comm_iterator_fn(handle_t l, node_t *data, node_t *node, void *arg, int *act)
{
    void *addr=(void*)(*(U32*)node->buf);
    
    comm_send_data(tasksHandle.hcom, addr, TYPE_CAP, 0, data->buf, data->dlen);
    *act = ACT_NONE;
    
    return 0;
}
static int comm_broadcast(handle_t list, node_t *data)
{
    return list_iterator(list, data, comm_iterator_fn, NULL);
}
///////////////////////////////////////////////////////////////////////////


void task_comm_send_fn(void *arg)
{
    int i,r,cnt;
    int  err=0;
    evt_t e;
    node_t node;
    
    LOGD("_____ task_comm_send running\n");
    while(1) {
        r = task_recv(TASK_COMM_SEND, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                                
                case EVT_COMM:
                {
                    while(1) {
                        r = list_get(tasksHandle.sendList, &node, 0);
                        if(r) {
                            break;
                        }
                        
                        err = comm_broadcast(tasksHandle.connList, &node);
                        list_remove(tasksHandle.sendList, 0);
                        //dal_delay_ms(5);
                    }
                }
                break;
            }
        }
    }
}
#endif


