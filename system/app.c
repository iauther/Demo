#include "task.h"
#include "log.h"
#include "app.h"
#include "mem.h"
#include "dal.h"
#include "upgrade.h"
#include "power.h"
#include "cfg.h"



int app_start(void)
{
    //dal_set_vector();
    dal_init();
    
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











