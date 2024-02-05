#include "at_port.h"


static int my_print(const char *fmt, ...)
{
    
}
static int my_io_init(void)
{
    
}
static int my_io_free(void)
{
    
}
static int my_io_read(void *data, int len)
{
    
}
static int my_io_write(void *data, int len)
{
    
}
static void* my_sem_init(void)
{
    
}
static int my_sem_free(void *h)
{
    
}
static int my_sem_hold(void *h)
{
    
}
static int my_sem_release(void *h)
{
    
}
static void* my_mtx_init)void()
{
    
}
static int my_mtx_free(void *h)
{
    
}
static int my_mtx_hold(void *h)
{
    
}
static int my_mtx_release(void *h)
{
    
}

static int my_tick_get(void)
{
    
}
static int my_delay_ms(int ms)
{
    
}

static int my_mem_malloc(int len)
{
    
}
static int my_mem_free(void *p)
{
    
}

at_port_t g_at_port={
    
    my_print,

    my_io_init,
    my_io_free,
    my_io_read,
    my_io_write,
    
    my_sem_init,
    my_sem_free,
    my_sem_hold,
    my_sem_release,
    
    my_mtx_init,
    my_mtx_free,
    my_mtx_hold,
    my_mtx_release,
    
    my_tick_get,
    my_delay_ms,

    my_mem_malloc,
    my_mem_free,
};