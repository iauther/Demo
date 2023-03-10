#ifndef __UPGRADE_Hx__
#define __UPGRADE_Hx__

#include "types.h"
#include "protocol.h"

enum {
    GOAL_BOOT=0,
    GOAL_APP,
};

int upgrade_is_need(void);
int upgrade_check(void);
U8 upgrade_proc(void *data);
U8 upgrade_is_finished(void);
U8 upgrade_is_app(void);

#endif




