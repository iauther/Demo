#include "at2.h"
#include "list.h"


typedef struct {
    at_dev_t    *dev;
    handle_t    list;
    notify_t    notify;
    char        used;
    char        dev_id;
}dev_handle_t;

typedef struct {
    U8           ctx;
    U8           conn;
    dev_handle_t *dev;
}at_conn_t;

typedef struct {
    at_sys_t     sys;
    dev_handle_t dev[AT_MAX];
}at_handle_t;
static at_handle_t atHandle={0}

static int at_notify(int evt, void *data, int len)
{
    
    return 0;
}
static void at_task(void *arg)
{
    dev_handle_t *dev=(dev_handle_t*)arg;
    
    while(1) {
        dev->recv();
        //at_notify();
    }
    
}




int at_init(at_sys_t *sys)
{
    atHandle.sys = *sys;
    
    return 0;
}


int at_deinit(void)
{
    return 0;
}


int at_install(int dev, at_dev_t *dev)
{
    int i=0,r=-1;
    
    for(i=0; i<AT_MAX; i++) {
        if(atHandle.dev[i].used==0) {
            dev_handle_t *dev=&atHandle.dev[i];
            dev->list = ;
            dev->dev = *dev;
            dev->used = 1;
            
            dev->init();
            dev->dev_id = i;
            atHandle.sys.task_init(at_task, dev);
            r = 0;
        }
    }
    
    return r;
}


int at_unstall(int dev)
{
    
    
}


void* at_conn(int dev, void *para)
{
    
}


int at_disconn(int dev, void *conn)
{
    
    
}




int at_send(int dev, void *conn, void *data, int len, int timeout)
{
    
}


int at_set_notify(int dev, notify_t notify)
{
    
}



















