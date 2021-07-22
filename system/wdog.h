#ifndef __WDOG_Hx__
#define __WDOG_Hx__

#include "types.h"

int wdog_init(void);
int wdog_feed(void);
int wdog_enable(U8 on);

#endif


