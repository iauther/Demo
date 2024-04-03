#include "dal_uart.h"
#include "list.h"
#include "log.h"
#include "cfg.h"
#include "mem.h"
#include "fs.h"
#include "lock.h"
#include "dal_rtc.h"


#define LOG_BUF_LEN  2000
#define LOG_NODE_MAX 1000

typedef struct {
    handle_t  hlog;
    int       enable;
    
    handle_t  lck;
    handle_t  list;
    log_cfg_t cfg;
    char      buffer[LOG_BUF_LEN];
}log_handle_t;


const char* log_string[LV_MAX]={
    "info",
    "debug",
    "warn",
    "error",
};


static log_handle_t logHandle={
    .hlog=NULL,
    .enable=1,
    
    .cfg={
        .en={1,1,1,1}
    },
};



int log_set(LOG_LEVEL lv, int en)
{
    lock_on(logHandle.lck);
    logHandle.cfg.en[lv] = en;
    lock_off(logHandle.lck);
    
    return 0;
}


int log_enable(int en)
{
    lock_on(logHandle.lck);
    logHandle.enable = en;
    lock_off(logHandle.lck);
    
    return 0;
}

int log_print(LOG_LEVEL lv, char *fmt, ...)
{
    int len;
    va_list args;
    datetime_t dt;
    
    if(!logHandle.enable || !logHandle.cfg.en[lv]) {
        return -1;
    }    
    
    lock_on(logHandle.lck);
    
    va_start(args, fmt);
    //dal_rtc_get(&dt);
    //vsnprintf(logHandle.buffer, LOG_BUF_LEN, "%02d:%02d:%02d, %s%s", dt.time.hour, dt.time.min, dt.time.sec, fmt, args);
    vsnprintf(logHandle.buffer, LOG_BUF_LEN, fmt, args);
    //vprintf(fmt, args);
    va_end(args);
    
    len = strlen(logHandle.buffer);
    dal_uart_write(logHandle.hlog, (U8*)logHandle.buffer, len);

#ifdef OS_KERNEL
    list_append(logHandle.list, lv, logHandle.buffer, len);
#endif
    lock_off(logHandle.lck);

    return 0;
}



int log_init(rx_cb_t callback)
{
    int i,r=0;
    dal_uart_cfg_t uc;

    if(logHandle.hlog) {
        return 0;
    }

    uc.mode = MODE_IT;
    uc.port = LOG_UART;
    uc.msb  = 0;
    uc.baudrate = LOG_BAUDRATE;
    uc.callback  = callback;
    uc.rx.blen = LOG_BUF_LEN;
    uc.rx.buf = malloc(uc.rx.blen);
    uc.rx.dlen = 0;
    
    logHandle.hlog = dal_uart_init(&uc);
    if(!logHandle.hlog) {
        return -1;
    }
    logHandle.list = NULL;
    logHandle.lck  = NULL;
    
    log_os_init();
    
    //LOGD("log_init\n");
    
    return 0;
}

int log_os_init(void)
{
#ifdef OS_KERNEL 
    list_cfg_t lc;
    lc.max  = LOG_NODE_MAX;
    lc.mode = LIST_FULL_FIFO;
    lc.log = 0;
    logHandle.list = list_init(&lc);
    
    logHandle.lck = lock_init();
#endif
    
    return 0;
}


int log_set_callback(rx_cb_t cb)
{
    int r;
    
    lock_on(logHandle.lck);
    r = dal_uart_set_callback(logHandle.hlog, 0, cb);
    lock_off(logHandle.lck);
    
    return r ;
}


int log_set_handle(handle_t h)
{
    lock_on(logHandle.lck);
    logHandle.hlog = h;
    lock_off(logHandle.lck);
    
    return 0;
}


handle_t log_get_handle(void)
{
    return logHandle.hlog;
}


int log_write(void *data, int len)
{
    int r;
    
    lock_on(logHandle.lck);
    r = dal_uart_write(logHandle.hlog, (U8*)data, len);
    lock_off(logHandle.lck);
    
    return r;
}


int log_save(void)
{
    int r=0;
    datetime_t dt;
    char tmp[100];
    handle_t hfile;
    list_node_t *lnode;
    handle_t list=logHandle.list;
    char *tok1="\n\n+++++++ log start +++++++\n";
    char *tok2="+++++++ log end   +++++++\n\n";

#ifdef OS_KERNEL
    if(list_size(list)==0) {
        r = -1;
        goto quit;
    }
    
    dal_rtc_get_time(&dt);
    sprintf(tmp, "%s/%04d/%02d/%02d/log.txt", SDMMC_MNT_PT, dt.date.year, dt.date.mon, dt.date.day);
    if(fs_exist(tmp)) {
        hfile = fs_open(tmp, FS_MODE_RW);
        if(hfile) {
            fs_seek(hfile, fs_length(tmp));
        }
    }
    else {
        hfile = fs_open(tmp, FS_MODE_CREATE);
    }
    
    if(!hfile) {
        r = -1;
        goto quit;
    }
    
    fs_write(hfile, tok1, strlen(tok1), 0);
    while(1) {
        r = list_get_node(list, &lnode, 0);
        if(r) {
            break;
        }
        fs_write(hfile, lnode->data.buf, lnode->data.dlen, 0);
        
        list_remove(list, 0);
    }
    fs_write(hfile, tok2, strlen(tok2), 0);
    
    fs_close(hfile);
#endif

quit:
    return r;
}



int log_deinit(void)
{
    int i;
    
#ifdef OS_KERNEL 
    list_free(logHandle.list);
    lock_free(logHandle.lck);
#endif
    
    dal_uart_deinit(logHandle.hlog);
    logHandle.hlog = NULL;
    logHandle.list = NULL;
    logHandle.lck  = NULL;
    
    return 0;
}





