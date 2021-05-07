#ifndef __PARA_Hx__
#define __PARA_Hx__

#include "data.h"


int paras_load(void);
int paras_read(U32 addr, void *data, U32 len);
int paras_write(U32 addr, void *data, U32 len);
int paras_write_node(node_t *n);

int paras_get_fwmagic(U32 *fwmagic);
int paras_set_fwmagic(U32 *fwmagic);
int paras_get_fwinfo(fw_info_t *fwinfo);
int paras_set_fwinfo(fw_info_t *fwinfo);

#endif

