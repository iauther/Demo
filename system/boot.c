#include "dal.h"
#include "boot.h"
#include "upgrade.h"
#include "dal_jump.h"
#include "dal_uart.h"
#include "dal_delay.h"
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
    dal_init();
    log_init(NULL);
    rtc2_init();
}

static void boot_deinit(void)
{
    log_deinit();
    
    upgrade_deinit();
}



int boot_start(void)
{
    int r=0;
    U32 runAddr=0;
    
    boot_init();
    LOGD("__boot start.\n");

#if 1
    upgrade_init();
    
    r = upgrade_check(&runAddr);
    if(r==0) {
        //boot_deinit();
        dal_jump_to_app(runAddr);
    }
#else

    int cntdwn=5;
    int runtime;
    
    date_time_t tm={
        .date = {
            .year  = 2023,
            .mon   = 10,
            .day   = 24,
            .week  = 2,
        },
        
        .time = {
            .hour = 8,
            .min  = 8,
            .sec  = 8,
        }
    };
    
    
    while(1) {
        
        runtime = rtc2_get_runtime();
        LOGD("____runtime: %d\n", runtime);
        
        if(runtime==3) {
            //rtc2_set_time(&tm);
        }
        
        if(runtime>=6) {
            LOGD("____ set countdown %d\n", cntdwn);
            rtc2_set_countdown(cntdwn);
            while(1);
        }
        dal_delay_ms(1000);
    }
    
    
    
#endif
    
    return 0;
}











