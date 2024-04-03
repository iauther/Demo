#include "dal.h"
#include "mem.h"
#include "rtc.h"
#include "log.h"
#include "cfg.h"
#include "boot.h"
#include "upgrade.h"

static void boot_init(void)
{
    log_init(NULL);
    rtc2_init();
}
static void boot_deinit(void)
{
    log_deinit();
    rtc2_deinit();
    upgrade_deinit();
    dal_deinit();
    rcu_deinit();
}



int boot_start(void)
{
    int r=0;
    U32 runAddr=APP_OFFSET+sizeof(fw_hdr_t);
    
    boot_init();
    LOGD("__boot start.\n");

   r = upgrade_proc(&runAddr, 1);
    if(r==0) {
        boot_deinit();
        dal_jump(runAddr);
    }
    
    return 0;
}











