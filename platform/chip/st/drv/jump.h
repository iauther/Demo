#ifndef __JUMP_Hx__
#define __JUMP_Hx__


#include "types.h"

typedef void (*jump_fn_t)(void);


void jump_to_app(void);

void jump_to_boot(void);


#endif

