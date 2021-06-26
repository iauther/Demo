#include "sys.h"
#include "task.h"
#include "paras.h"
#include "board.h"
 
int sys_init(void)
{
    board_init();
    paras_load();
    
#ifdef OS_KERNEL
    task_start();
#endif
    
    return 0;
}


