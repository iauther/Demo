#ifndef __POWER_Hx__
#define __POWER_Hx__

#include "types.h"

enum {
    
    PDEV_PAD=0,
    
    PDEV_MAX
};


int power_init(void);

int power_on(U8 pdev, U8 on);

#endif

