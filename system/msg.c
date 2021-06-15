#include <stdlib.h>
#include <string.h>
#include "msg.h"

#define FLAGS_MASK   0x00000001U


msg_t* msg_init(int max, int msg_size)
{
    msg_t *m=NULL;
    
    m = (msg_t*)calloc(1, sizeof(msg_t));
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


int msg_send(msg_t *m, void *ptr, int len)
{
    if(!m || !ptr || !len || len>m->msg_size) {
        return -1;
    }

#ifdef OS_KERNEL
    if(osMessageQueuePut(m->mq, ptr, 0, 0)!=osOK) {
        return -1;
    }
    osEventFlagsWait(m->ef, FLAGS_MASK, osFlagsWaitAny, osWaitForever);
#endif

    return 0;
}


int msg_post(msg_t *m, void *ptr, int len)
{
    if(!m || !ptr || !len || len>m->msg_size) {
        return -1;
    }

#ifdef OS_KERNEL
    if(osMessageQueuePut(m->mq, ptr, 0, 0)!=osOK) {
        return -1;
    }
#endif

    return 0;
}


int msg_recv(msg_t *m, void *ptr, int len)
{
    if(!m || !ptr || !len || len<m->msg_size) {
        return -1;
    }

#ifdef OS_KERNEL
    if(osMessageQueueGet(m->mq, ptr, NULL, osWaitForever)!=osOK) {
        return -1;
    }
#endif

    return 0;
}


int msg_ack(msg_t *m)
{
    int r=0;

    if(!m) {
        return -1;
    }

#ifdef OS_KERNEL
    if(m->ack) {
        r = (osEventFlagsSet(m->ef, FLAGS_MASK)==osOK)?0:-1;
    }
#endif

    return r;
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
