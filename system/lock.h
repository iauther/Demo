#ifndef __LOCK_Hx__
#define __LOCK_Hx__

#include "types.h"

enum {
    LOCK_IMEM=0,
    LOCK_MEM,
    LOCK_NOR,
    LOCK_SFLASH,
    
    LOCK_MAX
};

#ifdef __cplusplus
extern "C" {
#endif

int lock_staic_init(void);
int lock_static_free(void);
int lock_static_hold(int id);
int lock_static_release(int id);
/////////////////////////////////////////////////////
handle_t lock_dynamic_new(void);
int lock_dynamic_hold(handle_t h);
int lock_dynamic_release(handle_t h);
int lock_dynamic_free(handle_t h);

#ifdef __cplusplus
}
#endif


#endif




