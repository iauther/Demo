#include "dal.h"
#include "boot.h"
#include "upgrade.h"
#include "dal_jump.h"
#include "dal_uart.h"
#include "rs485.h"
#include "ecxxx.h"
#include "rtc.h"
#include "log.h"



#define USE_UART


#define RECV_BUF_LEN   1000



static int rs485_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    
    return 0;
}
static int ecxxx_recv_callback(void *data, int len)
{
    
    return 0;
}

static void boot_init(void)
{
    rs485_cfg_t rc;
    ecxxx_cfg_t ec;
    
    dal_init();
    log_init(NULL);
    rtcx_init();
   

#ifdef USE_UART
    rc.port = UART_2;
    rc.baudrate = 115200;
    rc.callback = rs485_recv_callback;
    rs485_init(&rc);
    
    ec.port = UART_1;
    ec.baudrate = 115200;
    ec.callback = ecxxx_recv_callback;
    ecxxx_init(&ec);
#endif
    
    upgrade_init();
}

static void boot_deinit(void)
{
    log_deinit();
    rs485_deinit();
    ecxxx_deinit();
    
    upgrade_deinit();
}


int boot_start(void)
{
    int r=0;
    U32 runAddr=0;
    
    boot_init();
    LOGD("__boot start.\n");
    
    r = upgrade_check(&runAddr);
    if(r==0) {
        //boot_deinit();
        dal_jump_to_app(runAddr);
    }

#if 0
    while(1) {
        
        
        
    }
#endif
    
    return 0;
}











