#ifndef __NVM_Hx__
#define __NVM_Hx__

#include "types.h"

int nvm_init(void);
int nvm_read(U32 addr, U8 *data, U32 len);
int nvm_write(U32 addr, U8 *data, U32 len);
int nvm_erase(U32 addr, U8 *data, U32 len);

#endif


