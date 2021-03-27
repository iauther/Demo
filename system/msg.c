#include <stdlib.h>
#include <string.h>
#include "msg.h"

#define FLAGS_MASK   0x00000001U

static int msg_put(msg_t *m, void *ptr, int len)
{
    void *p;

#ifdef OS_KERNEL
    osStatus_t st;
    p = osMemoryPoolAlloc(m->mp, 0);
    if(!p) {
        return -1;
    }
    
    memcpy(p, ptr, len);
    st = osMessageQueuePut(m->mq, &p, 0, 0);
    if(st!=osOK) {
        return -1;
    }
#endif
   
    return 0;
}

msg_t* msg_init(int max, int msg_size)
{
    msg_t *m=NULL;
    
    m = (msg_t*)calloc(1, sizeof(msg_t));
    if(!m) {
        return NULL;
    }

#ifdef OS_KERNEL
    m->mq = osMessageQueueNew(max, sizeof(void*), NULL);
    m->mp = osMemoryPoolNew(max, msg_size, NULL);
    m->ef = osEventFlagsNew(NULL);
    m->ack = 0;
    m->msg_size = msg_size;
#endif

    return m;
}


int msg_send(msg_t *m, void *ptr, int len)
{
    int r;
    
    if(!m || !ptr || !len || len>m->msg_size) {
        return -1;
    }

#ifdef OS_KERNEL
    r = msg_put(m, ptr, len);
    if(r==0) {
        osEventFlagsWait(m->ef, FLAGS_MASK, osFlagsWaitAny, osWaitForever);
    }
#endif

    return r;
}


int msg_post(msg_t *m, void *ptr, int len)
{
    int r=0;

    if(!m || !ptr || !len || len>m->msg_size) {
        return -1;
    }

#ifdef OS_KERNEL
    r = msg_put(m, ptr, len);
#endif

    return r;
}


int msg_recv(msg_t *m, void *ptr, int len)
{
    int r=0;
    void *p=NULL;

    if(!m || !ptr || !len || len<m->msg_size) {
        return -1;
    }

#ifdef OS_KERNEL
    osStatus_t st;
    st = osMessageQueueGet(m->mq, &p, NULL, osWaitForever);
    if(st!=osOK || !p) {
        return -1;
    }
    
    memcpy(ptr, p, m->msg_size);
    osMemoryPoolFree(m->mp, p);
#endif

    return r;
}


int msg_ack(msg_t *m)
{
    int r=0;

    if(!m) {
        return -1;
    }

#ifdef OS_KERNEL
    osStatus_t st;
    if(m->ack) {
        osEventFlagsSet(m->ef, FLAGS_MASK);
        r = (st==osOK)?0:-1;
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
    osStatus_t st;
    st = osMessageQueueReset(m->mq);
    r = (st==osOK)?0:-1;
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
    osMemoryPoolDelete((*m)->mp);
    osEventFlagsDelete((*m)->ef);
    free(*m);
#endif

    return 0;
}
