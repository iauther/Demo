#ifndef __SFLASH_Hx__
#define __SFLASH_Hx__

#include "types.h"

int sflash_init(void);
int sflash_deinit(void);
int sflash_erase_all(void);
int sflash_erase(U32 addr, U32 length);
int sflash_erase_sector(U32 sec);
int sflash_write(U32 addr, void* buf, U32 len, U8 check);
int sflash_read(U32 addr, void* buf, U32 len);

int sflash_test(void);
#endif

