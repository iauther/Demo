#ifndef __PORT_Hx__
#define __PORT_Hx__

#include "types.h"


int port_init(void);
int port_free(void);

int port_open(int port);
int port_close(void);
int port_is_opened(void);

int port_send(void* data, U16 len);
int port_recv(void* data, U16 len);

#endif

