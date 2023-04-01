#ifndef __CAP_Hx__
#define __CAP_Hx__

#include "types.h"
#include "dal/adc.h"
#include "dal/sai.h"
#include "dal/tmr.h"

typedef void (*cap_callback_t)(U8 *data, U32 len);


typedef struct {
    cap_callback_t adc_fn;
    cap_callback_t tmr_fn;
    cap_callback_t sai_fn;
}cap_cfg_t;

int cap_init(void);
int cap_config(cap_cfg_t *cfg);
int cap_start(void);
int cap_stop(void);

#endif
