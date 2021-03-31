#ifndef __RBUF_Hx__
#define __RBUF_Hx__

#include "types.h"


handle_t rbuf_init(void *buf, int size);

int rbuf_free(handle_t *h);

int rbuf_read(handle_t h, U8 *buf, int len);

int rbuf_write(handle_t h, U8 *buf, int len);

int rbuf_get_size(handle_t h);

int rbuf_get_dlen(handle_t h);

#endif
