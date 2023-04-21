#ifndef __DAL_SAI_Hx__
#define __DAL_SAI_Hx__

#include "types.h"
#include "dal_mdma.h"


typedef struct {
    U8      e3Mux;
    U16     chBits;
    
    mdma_cfg_t mdma;
}dal_sai_cfg_t;


int dal_sai_init(void);
int dal_sai_config(dal_sai_cfg_t *cfg);
int dal_sai_start(void);
int dal_sai_stop(void);

#endif



