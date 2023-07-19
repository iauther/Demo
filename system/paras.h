#ifndef __PARA_Hx__
#define __PARA_Hx__

#include "protocol.h"


int paras_load(void);
int paras_erase(void);
int paras_reset(void);
int paras_read(int id, void *data, int len);
int paras_write(int id, void *data, int len);

int paras_get_fwinfo(fw_info_t *fwinfo);
int paras_set_fwinfo(fw_info_t *fwinfo);

extern U8 sysState;
extern U8 sysMode;
extern state_t curStat;
extern para_t  curPara;
extern paras_t DEFAULT_PARAS;
extern char *chJson;

#endif

