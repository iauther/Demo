#include "dal_uart.h"
#include "log.h"
#include "cfg.h"


handle_t logHandle=NULL;
const char* log_string[LV_MAX]={
    "-----info-----",
    "----debug-----",
    "---warnning---",
    "----error-----",
};
log_cfg_t log_cfg={

    {
        1,
        1,
        1,
        1,
    }
};

int log_get(log_cfg_t *cfg)
{
    if(!cfg) {
        return -1;
    }
    
    *cfg = log_cfg;
    
    return 0;
}


int log_set(log_cfg_t *cfg)
{
    if(!cfg) {
        return -1;
    }
    
    if(memcmp(cfg, &log_cfg, sizeof(log_cfg_t))) {
        log_cfg = *cfg;
    }
    
    return 0;
}


int log_print(LEVEL lv, char *fmt, ...)
{
    va_list args;

    if(log_cfg.en[lv]) {
        //printf("%s %s %d ", log_string[lv], __FILE__, __LINE__);
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }

    return 0;
}



int log_init(rx_cb_t callback)
{
    int r=0;
    dal_uart_cfg_t uc;

    uc.mode = MODE_IT;
    uc.port = LOG_UART;
    uc.msb  = 0;
    uc.baudrate = LOG_BAUDRATE;
    uc.para.callback  = callback;
    uc.para.buf = NULL;
    uc.para.blen = 0;
    uc.para.dlen = 0;
    logHandle = dal_uart_init(&uc);
    if(!logHandle) {
        return -1;
    }
    
    return 0;
}


int log_set_callback(rx_cb_t cb)
{
    return dal_uart_set_callback(logHandle, cb);
}




int log_deinit(void)
{
    return dal_uart_deinit(logHandle);
}





