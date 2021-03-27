#ifndef __FLASH_Hx__
#define __FLASH_Hx__

#include "types.h"

int flash_read(U32 offset, U8 *data, U32 len);

int flash_write(U32 offset, U8 *data, U32 len);

int flash_erase(U32 from, U32 to);

#endif
