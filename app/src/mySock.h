#ifndef __MYSOCK_Hx__
#define __MYSOCK_Hx__

#include "hv.h"
#include "TcpClient.h"
#include "UdpClient.h"

#define SOCK_BUF_LEN  50000

class mySock
{
public:
    mySock();
    ~mySock();

    int conn(char *ip, int port);
    int disconn(void);
    int read(void* data, int len);
    int write(void *data, int len);
    
private:
    hv::TcpClient   tc;
    hv::UdpClient   uc;
    
    htimed_mutex_t mutex;

    int         datalen;
    char        buffer[SOCK_BUF_LEN];
};

#endif

