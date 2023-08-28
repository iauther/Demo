#include "task.h"
#include "log.h"
#include "app.h"
#include "dal.h"
#include "upgrade.h"
#include "protocol.h"
#include "power.h"
#include "cfg.h"



int app_start(void)
{
    //dal_set_vector();
    dal_init();
    //log_init(NULL);
    
    LOGD("\n__app start\n");
    
    power_init();
    upgrade_check(NULL);
    
#ifdef OS_KERNEL
    task_init();
#else
    test_main();
#endif
    
    return 0;
}











