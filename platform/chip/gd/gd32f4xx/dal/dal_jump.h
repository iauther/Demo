#ifndef __DAL_JUMP_Hx__
#define __DAL_JUMP_Hx__


#include "types.h"

void dal_reboot(void);
void dal_jump_to_app(U32 addr);
void dal_jump_to_boot(U32 addr);


#endif

