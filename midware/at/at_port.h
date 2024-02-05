#ifndef __AT_PORT_Hx__
#define __AT_PORT_Hx__


typedef struct {
    int (*print)(const char *fmt, ...);
    int (*tick_get)(void);
    int (*delay_ms)(int ms);
    
    //io
    void* (*io_init)(void);
    int (*io_free)(void *h);
    int (*io_read)(void *h, void *data, int len);
    int (*io_write)(void *h, void *data, int len);
    
    //semphore
    void* (*sem_init)(void);
    int (*sem_free)(void *h);
    int (*sem_hold)(void *h);
    int (*sem_release)(void *h);
    
    //mutex
    void* (*mtx_init)(void);
    int (*mtx_free)(void *h);
    int (*mtx_hold)(void *h);
    int (*mtx_release)(void *h);
    
    int (*tmr_init)();
    int (*tmr_free)(void *h);
    
    int (*task_init)();
    int (*task_free)(void *h);
    
    int (*mem_malloc)(int len);
    int (*mem_free)(void *p);
}at_port_t;

extern at_port_t g_at_port;
#define AT_PORT ((at_port_t*)&g_at_port)

#endif


