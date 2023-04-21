#ifndef __DAL_FLASH_Hx__
#define __DAL_FLASH_Hx__

#include "types.h"

int dal_flash_init(void);
int dal_flash_read(U32 offset, U8 *data, U32 len);
int dal_flash_write(U32 offset, U8 *data, U32 len);
int dal_flash_erase(U32 from, U32 to);
int dal_flash_test(void);
#endif
