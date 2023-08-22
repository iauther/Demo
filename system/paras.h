#ifndef __PARA_Hx__
#define __PARA_Hx__

#include "protocol.h"


int paras_load(void);
int paras_reset(void);

void paras_set_state(int state);
int paras_get_state(void);
int paras_get_run_datetime(date_time_t *dt);

extern const char *all_para_ini;
extern const char *all_para_json;
extern const char *filesPath[];
extern const all_para_t DFLT_PARA;

extern all_para_t allPara;

#endif

