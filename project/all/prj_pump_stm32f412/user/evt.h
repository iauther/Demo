#ifndef __EVT_Hx__
#define __EVT_Hx__

#include "types.h"

#define MAXLEN  128

enum {
    EVT_COM,
    EVT_N950,
    EVT_MS4525,

    EVT_TIMER,
};

typedef struct {
    U8      evt;
    U8      type;
    U8      dLen;
    U8      data[MAXLEN];
}evt_t;

#endif


