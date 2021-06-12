#ifndef __UPGRADE_Hx__
#define __UPGRADE_Hx__

#include "types.h"
#include "data.h"

enum {
    OBJ_BOOT=0,
    OBJ_APP,
};

int upgrade_get_fwmagic(U32 *fwmagic);
int upgrade_set_fwmagic(U32 *fwmagic);
int upgrade_get_fwinfo(fw_info_t *fwinfo);
int upgrade_set_fwinfo(fw_info_t *fwinfo);

int upgrade_is_need(void);
int upgrade_init(U8 obj);
int upgrade_check(void);
int upgrade_write(U8 *data, U32 len);
int upgrade_refresh(void);
U8 upgrade_proc(void *data);

#endif




