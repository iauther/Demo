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

int upgrade_prep(U8 obj, U32 fwlen);
int upgrade_check(void);
int upgrade_write(U8 *data, U32 len);

#endif




