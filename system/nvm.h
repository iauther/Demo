#ifndef __NVM_Hx__
#define __NVM_Hx__

#include "types.h"

int nvm_init(void);
int nvm_read(int id,  void *data, int len);
int nvm_write(int id, void *data, int len);
int nvm_remove(int id);

#endif


