#ifndef __ADS8866_Hx__
#define __ADS8866_Hx__

#include "types.h"

int ads8866_init(void);

int ads8866_read(U8 *data, U32 len);

int ads8866_check(void);

int ads8866_reset(void);

#endif

