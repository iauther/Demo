#ifndef __ADS9120_Hx__
#define __ADS9120_Hx__

#include "types.h"
#include "dal_spi.h"

typedef void (*ads_data_t)(U16 *data, U32 len);

typedef struct {
    U32         freq;
    spi_buf_t   buf;
    
    ads_data_t callback;
}ads9120_cfg_t;



int ads9120_init(ads9120_cfg_t *cfg);
int ads9120_read(S16 *data, U32 cnt);
int ads9120_conv(F32 *f, U16 *u, U32 cnt);
int ads9120_power(U8 on);
int ads9120_test(void);
#endif
