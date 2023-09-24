#include "dal_uart.h"
#include "list.h"
#include "log.h"
#include "cfg.h"
#include "fs.h"
#include "dal_rtc.h"


#define LOG_BUF_LEN  2000

typedef struct {
    lv_cfg_t  cfg;
    handle_t  list;
}lv_data_t;

typedef struct {
    handle_t  hlog;
    int       enable;
    lv_data_t lv[LV_MAX];
    
    char      buffer[LOG_BUF_LEN];
}log_handle_t;


const char* log_string[LV_MAX]={
    "---info---",
    "---debug---",
    "---warn---",
    "---error---",
};


static log_handle_t logHandle={
    .hlog=NULL,
    .enable=1,
    
    .lv={
        {1,500,NULL},{1,500,NULL},{1,500,NULL},{1,500,NULL}
    },
};



int log_set(LOG_LEVEL lv, int en)
{
    int i;
    
    for(i=0; i<LV_MAX; i++) {
        logHandle.lv[i].cfg.en = en;
    }
    
    return 0;
}


int log_enable(int en)
{
    logHandle.enable = en;
    return 0;
}

int log_print(LOG_LEVEL lv, char *fmt, ...)
{
    int len;
    va_list args;
    date_time_t dt;

    if(!logHandle.enable || !logHandle.lv[lv].cfg.en) {
        return -1;
    }    
    
    va_start(args, fmt);
    //dal_rtc_get(&dt);
    //vsnprintf(logHandle.buffer, LOG_BUF_LEN, "%02d:%02d:%02d, %s%s", dt.time.hour, dt.time.min, dt.time.sec, fmt, args);
    vsnprintf(logHandle.buffer, LOG_BUF_LEN, fmt, args);
    //vprintf(fmt, args);
    va_end(args);
    
    len = strlen(logHandle.buffer);
    dal_uart_write(logHandle.hlog, (U8*)logHandle.buffer, len);

#ifndef BOOTLOADER 
    list_append(logHandle.lv[lv].list, logHandle.buffer, len);
#endif

    return 0;
}



int log_init(rx_cb_t callback)
{
    int i,r=0;
    list_cfg_t lc;
    dal_uart_cfg_t uc;

    if(logHandle.hlog) {
        return 0;
    }
    
    uc.mode = MODE_IT;
    uc.port = LOG_UART;
    uc.msb  = 0;
    uc.baudrate = LOG_BAUDRATE;
    uc.para.callback  = callback;
    uc.para.buf = NULL;
    uc.para.blen = 0;
    uc.para.dlen = 0;
    logHandle.hlog = dal_uart_init(&uc);
    if(!logHandle.hlog) {
        return -1;
    }
    
#ifndef BOOTLOADER 
    for(i=0; i<LV_MAX; i++) {
        lc.max = logHandle.lv[i].cfg.max;
        lc.mode = MODE_FIFO;
        logHandle.lv[i].list = list_init(&lc);
    }
#endif
    
    return 0;
}


int log_set_callback(rx_cb_t cb)
{
    return dal_uart_set_callback(logHandle.hlog, cb);
}


int log_set_handle(handle_t h)
{
    logHandle.hlog = h;
    return 0;
}


handle_t log_get_handle(void)
{
    return logHandle.hlog;
}


int log_save(LOG_LEVEL lv)
{
    int r;
    node_t node;
    date_time_t dt;
    char tmp[100];
    handle_t hfile;
    handle_t list=logHandle.lv[lv].list;

#ifndef BOOTLOADER    
    if(list_size(list)==0) {
        return -1;
    }
    
    dal_rtc_get(&dt);
    sprintf(tmp, "%s/%04d/%02d/%02d/log.txt", SDMMC_MNT_PT, dt.date.year, dt.date.mon, dt.date.day);
    
    fs_remove(tmp);
    hfile = fs_open(tmp, FS_MODE_CREATE);
    if(!hfile) {
        LOGE("___ %s create failed\n");
        return -1;
    }
    
    while(1) {
        r = list_get(list, &node, 0);
        if(r) {
            break;
        }
        fs_write(hfile, node.buf, node.dlen, 0);
        
        list_remove(list, 0);
    }
    fs_close(hfile);
#endif
    
    return 0;
}


int log_save_all(void)
{
    int i;
    
    for(i=0; i<LV_MAX; i++) {
        log_save((LOG_LEVEL)i);
    }
    
    return 0;
}


int log_deinit(void)
{
    int i;
    
#ifndef BOOTLOADER 
    for(i=0; i<LV_MAX; i++) {
        list_free(logHandle.lv[i].list);
    }
#endif
    
    dal_uart_deinit(logHandle.hlog);
    logHandle.hlog = NULL;
    
    return 0;
}





