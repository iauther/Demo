#ifndef __LOCK_Hx__
#define __LOCK_Hx__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif


handle_t lock_init(void);
int lock_on(handle_t h);
int lock_off(handle_t h);
int lock_free(handle_t h);

#ifdef __cplusplus
}
#endif


#endif




