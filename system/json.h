#ifndef __JSON_Hx__
#define __JSON_Hx__

#include "types.h"
#include "protocol.h"


int json_init(void);
int json_to_stat(int size, int zero);
int json_from(int size, int zero, int alignment);

#endif

