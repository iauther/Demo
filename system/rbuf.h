#ifndef __RBUF_Hx__
#define __RBUF_Hx__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    RBUF_FULL_FIFO=0,
    RBUF_FULL_FILO,
};


typedef struct {
    void *buf;
    int size;
    int mode;
}rbuf_cfg_t;


handle_t rbuf_init(rbuf_cfg_t *cfg);
int rbuf_free(handle_t h);
int rbuf_read(handle_t h, void *buf, int len, U8 shift);
int rbuf_read_shift(handle_t h, int len);
int rbuf_write(handle_t h, void *buf, int len);
int rbuf_clear(handle_t h);
int rbuf_size(handle_t h);
int rbuf_get_dlen(handle_t h);
int rbuf_is_full(handle_t h);

#ifdef __cplusplus
}
#endif


#endif
