#ifndef __NETIO_Hx__
#define __NETIO_Hx__

#include "types.h"

enum {
    NETIO_SPI=0,
    NETIO_USB,
    NETIO_ETH,
    NETIO_UART,
    NETIO_ECXXX,
    
    NETIO_MAX
};

typedef struct {
    U8      io;
    void    *para;
}netio_cfg_t;

handle_t netio_init(netio_cfg_t *cfg);
int netio_deinit(handle_t hio);
int netio_write(handle_t hio, void *data, int len);
int netio_write(handle_t hio, void *data, int len);

#endif

