#ifndef __RBUF_Hx__
#define __RBUF_Hx__

#include "types.h"

typedef struct {
	int     pr;
	int     pw;
	int     dlen;
	int     size;
    U8      *buf;
}rbuf_t;


int rbuf_init(rbuf_t *rb, void *buf, int size);

int rbuf_free(rbuf_t *rb);

int rbuf_read(rbuf_t *rb, U8 *buf, int len);

int rbuf_write(rbuf_t *rb, U8 *buf, int len);

int rbuf_get_size(rbuf_t *rb);

int rbuf_get_dlen(rbuf_t *rb);

#endif
