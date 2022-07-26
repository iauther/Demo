#include "sys2.h"
#include "task.h"
#include "paras.h"
#include "board.h"
 
int sys_init(void)
{
    board_init();
    
#ifdef OS_KERNEL
    task_start();
#endif
    
    return 0;
}


