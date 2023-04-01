#ifndef __RBUF_Hx__
#define __RBUF_Hx__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

handle_t rbuf_init(void *buf, int size);

int rbuf_free(handle_t h);

int rbuf_read(handle_t h, U8 *buf, int len, U8 shift);

int rbuf_read_shift(handle_t h, int len);

int rbuf_write(handle_t h, U8 *buf, int len);

int rbuf_get_size(handle_t h);

int rbuf_get_dlen(handle_t h);

#ifdef __cplusplus
}
#endif


#endif
