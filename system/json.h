#ifndef __JSON_Hx__
#define __JSON_Hx__

#include "types.h"
#include "protocol.h"

int json_init(void);
int json_from_ch(char *js, int js_len, ch_cfg_t *cfg, int cfg_cnt);
int json_from_ev(char *js, int js_len, ev_cfg_t *cfg, int cfg_cnt);
int json_to_ch(char *js, ch_cfg_t *cfg, int cfg_cnt);
int json_to_ev(char *js, ev_cfg_t *cfg, int cfg_cnt);

#endif

