#ifndef __EVT_Hx__
#define __EVT_Hx__

#include "types.h"

enum {
    EVT_COM,
    EVT_N950,
    EVT_MS4525,

    EVT_TIMER,
};

typedef struct {
    U8      evt;
    U8      type;
    U8      dataLen;
    U8      data[30];
}evt_t;

typedef struct {
    U8      evt;
    U8      type;
    U8      dataLen;
    U8      data[256];
}evt2_t;

#endif


