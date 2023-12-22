#include "task.h"
#include "app.h"
#include "dal.h"

int app_start(void)
{
    dal_init();
    
#ifdef OS_KERNEL
    task_init();
#else
    test_main();
#endif
    
    return 0;
}











