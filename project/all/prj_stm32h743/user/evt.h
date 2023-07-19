#ifndef __EVT_Hx__
#define __EVT_Hx__

#include "types.h"

#define EVT_DATA_LEN_MAX   0xFF

enum {
    EVT_ADC=0,
    EVT_SAI,
    EVT_TMR,
    EVT_COMM,
    EVT_DATA,
    
    EVT_TIMER,
    EVT_SAVE,
};

typedef struct {
    void    *arg;
    U8      evt;
    U8      type;
    U16     flag;
    U32     dLen;
    U8      data[EVT_DATA_LEN_MAX];
}evt_t;

#endif


