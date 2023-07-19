#include "board.h"
#include "lock.h"
#include "fs.h"
#include "paras.h"
#include "json.h"

int board_init(void)
{
    int r=0;

    lock_staic_init();
    paras_load();
    json_init();
    
    return 0;
}


int board_deinit(void)
{
    int r;
    
    
    
    return 0;
}


