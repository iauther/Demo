#ifndef __SAI_Hx__
#define __SAI_Hx__

#include "types.h"
#include "dal/mdma.h"


typedef struct {
    U8      e3Mux;
    U16     chBits;
    
    mdma_cfg_t mdma;
}sai_cfg_t;


int sai_init(void);
int sai_config(sai_cfg_t *cfg);
int sai_start(void);
int sai_stop(void);

#endif



