#ifndef __PARA_Hx__
#define __PARA_Hx__

#include "data.h"
#include "notice.h"


int paras_load(void);
int paras_erase(void);
int paras_reset(void);
int paras_read(U32 addr, void *data, U32 len);
int paras_write(U32 addr, void *data, U32 len);
int paras_write_node(node_t *n);

int paras_get_magic(U32 *magic);
int paras_set_magic(U32 *magic);
int paras_get_state(U8 *state);
int paras_set_state(U8 state);
int paras_get_fwinfo(fw_info_t *fwinfo);
int paras_set_fwinfo(fw_info_t *fwinfo);
int paras_set_upg(void);
int paras_clr_upg(void);


extern U8 sysState;
extern U8 sysMode;
extern stat_t curStat;
extern para_t curPara;
extern notice_t allNotice[LEV_MAX];

#endif

