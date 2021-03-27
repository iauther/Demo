#ifndef __MS4525_Hx__
#define __MS4525_Hx__

#include "types.h"

#pragma pack (1)

typedef struct {
    F32     pres;
    F32     temp;
}ms4525_t;

#pragma pack () 

int ms4525_init(void);

int ms4525_get(ms4525_t *m);

void ms4525_test(void);

#endif



