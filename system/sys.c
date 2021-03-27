#include "sys.h"
#include "task.h"
#include "board.h"
 
int sys_init(void)
{
    board_init();
    
#ifdef OS_KERNEL
    task_start();
#else
    //task_dev_fn(0);
#endif
    
    return 0;
}


