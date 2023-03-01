#ifndef __NET_Hx__
#define __NET_Hx__

#include "lwip.h"
#include "devs.h"
#include "dal/dal.h"

enum {
    NET_ETH=0,
    NET_WIFI,

    NET_MAX,
};


#define DHCP_OFF                   (uint8_t) 0
#define DHCP_START                 (uint8_t) 1
#define DHCP_WAIT_ADDRESS          (uint8_t) 2
#define DHCP_ADDRESS_ASSIGNED      (uint8_t) 3
#define DHCP_TIMEOUT               (uint8_t) 4
#define DHCP_LINK_DOWN             (uint8_t) 5


int net2_init(void);

int net2_conn(void);

int net2_read(U8 *data, int len);

int net2_write(U8 *data, int len);

int net2_test(void);

#endif
