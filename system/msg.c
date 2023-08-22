#include <stdlib.h>
#include <string.h>
#include "msg.h"

#define FLAGS_MASK   0x00000001U


msg_t* msg_init(int max, int msg_size)
{
    msg_t *m=NULL;
    
    m = (msg_t*)malloc(sizeof(msg_t));
    if(!m) {
        return NULL;
    }

#ifdef OS_KERNEL
    m->mq = osMessageQueueNew(max, msg_size, NULL);
    m->ef = osEventFlagsNew(NULL);
    m->ack = 0;
    m->msg_size = msg_size;
#endif

    return m;
}


int msg_send(msg_t *m, void *ptr, int len, U32 timeout)
{
    osStatus_t st;
    
    if(!m || !ptr || !len || len>m->msg_size) {
        return -1;
    }

#ifdef OS_KERNEL
    st = osMessageQueuePut(m->mq, ptr, 0, 0);
    if(st!=osOK) {
        return st;
    }
    osEventFlagsWait(m->ef, FLAGS_MASK, osFlagsWaitAny, timeout);
#endif

    return 0;
}


int msg_post(msg_t *m, void *ptr, int len)
{
    osStatus_t st;
    
    if(!m || !ptr || !len || len>m->msg_size) {
        return -1;
    }

#ifdef OS_KERNEL
    st = osMessageQueuePut(m->mq, ptr, 0, 0);
    if(st!=osOK) {
        return -1;
    }
#endif

    return 0;
}


int msg_recv(msg_t *m, void *ptr, int len)
{
    osStatus_t st;
    
    if(!m || !ptr || !len || len<m->msg_size) {
        return -1;
    }

#ifdef OS_KERNEL
    st = osMessageQueueGet(m->mq, ptr, NULL, osWaitForever);
    if(st!=osOK) {
        return st;
    }
    
    if(m->ack) {
        osEventFlagsSet(m->ef, FLAGS_MASK);
    }
#endif

    return 0;
}



int msg_reset(msg_t *m)
{
    int r=0;

    if(!m) {
        return -1;
    }

#ifdef OS_KERNEL
    r = (osMessageQueueReset(m->mq)==osOK)?0:-1;
#endif

    return r;
}


int msg_free(msg_t **m)
{
    if(!m || !(*m)) {
        return -1;
    }

#ifdef OS_KERNEL
    osMessageQueueDelete((*m)->mq);
    osEventFlagsDelete((*m)->ef);
    free(*m);
#endif

    return 0;
}
