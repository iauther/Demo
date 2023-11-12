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
int paras_set_state(U8 ch, U8 stat);
int paras_get_state(U8 ch);
int paras_set_finished(U8 ch, U8 f);
int paras_is_finished(U8 ch);

cali_t* paras_get_cali(U8 ch);
int paras_set_coef(U8 ch, coef_t *coef);
int paras_set_cali_sig(U8 ch, cali_sig_t *sig);
ch_para_t* paras_get_ch_para(U8 ch);
ch_paras_t* paras_get_ch_paras(U8 ch);
int paras_get_smp_points(U8 ch);
U8 paras_get_port(void);

int paras_set_mode(U8 mode);
U8 paras_get_mode(void);

int paras_save(void);
int paras_factory(void);
int paras_check(all_para_t *all);

extern const char* filesPath[];
extern const all_para_t DFLT_PARA;
extern const ch_para_t DFLT_TEST_PARA[CH_MAX];
extern const date_time_t DFLT_TIME;
extern all_para_t allPara;

#ifdef __cplusplus
}
#endif

#endif

