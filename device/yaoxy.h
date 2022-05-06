#ifndef __YAOXY_H__
#define __YAOXY_H__

#include "types.h"

enum {
    YAOXY_CMD=0,
    YAOXY_DAT,
};

enum {
    ID_ASCII=0,
    ID_CHINESE,
};

typedef struct {
    U16  x;
    U16  y;
}pos_t;

int yaoxy_init(void);

int yaoxy_write(U16 x, U16 y, U8 *data);


#endif
