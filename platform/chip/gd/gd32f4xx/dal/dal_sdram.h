#ifndef __DAL_SDRAM_Hx__
#define __DAL_SDRAM_Hx__

#include "types.h"

#define SDRAM_ADDR   0xC0000000
#define SDRAM_LEN    (32*1024*1024)

int dal_sdram_init(void);

#endif

