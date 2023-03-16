#ifndef __EVT_Hx__
#define __EVT_Hx__

#include "types.h"

#define EVT_DATA_LEN_MAX   0xFF

enum {
    EVT_COM=0,
    EVT_ADC,
    
    EVT_TIMER,
    EVT_EEPROM,
};

typedef struct {
    void    *addr;
    U8      evt;
    U8      type;
    U8      dLen;
    U8      data[EVT_DATA_LEN_MAX];
}evt_t;

#endif


