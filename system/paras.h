#ifndef __PARA_Hx__
#define __PARA_Hx__

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

int paras_load(void);
int paras_reset(void);

int paras_set_usr(usr_para_t *usr);
smp_para_t* paras_get_smp(void);
void paras_set_state(int state);
int paras_get_state(void);
int paras_get_prev_state(void);
int paras_state_restore(void);

cali_t* paras_get_cali(U8 ch);
int paras_set_coef(U8 ch, coef_t *coef);
int paras_set_cali_sig(U8 ch, cali_sig_t *sig);
ch_para_t* paras_get_ch_para(U8 ch);
int paras_get_datetime(date_time_t* dt);
int paras_get_smp_cnt(U8 ch);
int paras_save(void);

extern const char* all_para_ini;
extern const char* all_para_json;
extern const char* filesPath[];
extern const all_para_t DFLT_PARA;
extern const date_time_t DFLT_TIME;
extern all_para_t allPara;

#ifdef __cplusplus
}
#endif

#endif

