#ifndef __DAL_CAP_Hx__
#define __DAL_CAP_Hx__

#include "types.h"
#include "dal_adc.h"
#include "dal_sai.h"
#include "dal_tmr.h"

typedef void (*cap_callback_t)(U8 *data, U32 len);


typedef struct {
    cap_callback_t adc_fn;
    cap_callback_t tmr_fn;
    cap_callback_t sai_fn;
}dal_cap_cfg_t;

int dal_cap_init(void);
int dal_cap_config(dal_cap_cfg_t *cfg);
int dal_cap_start(void);
int dal_cap_stop(void);

#endif
