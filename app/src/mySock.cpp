#include <stdio.h>
#include "mySock.h"
#include "log.h"

#pragma comment(lib, "lib/hv_static.lib")

using namespace hv;


static void on_message(const SocketChannelPtr& channel, Buffer* buf)
{
    //htimed_mutex_lock(&mutex);
        //recv_fn(buf->data(), buf->size());
    //htimed_mutex_unlock(&mutex);
}


mySock::mySock()
{
    datalen = 0;
}

mySock::~mySock()
{

}


void* mySock::conn(char *ip, int port)
{
    int fd;
    reconn_setting_t rc;
    hv::TcpClient tc;
    
    fd = tc.createsocket(port, ip);
    if(fd<0) {
        LOGE("___ createsocket failed\n");
        return NULL;
    }
    
    datalen = 0;
	tc.onMessage = on_message;
    htimed_mutex_init(&mutex);
	
    reconn_setting_init(&rc);
    rc.min_delay = 1000;
    rc.max_delay = 10000;
    rc.delay_policy = 2;
    tc.setReconnect(&rc);
	
    tc.start();
	    
    return NULL;
}


int mySock::disconn(void* conn)
{
    hv::TcpClient tc;

    tc.stop();
    tc.closesocket();
    htimed_mutex_destroy(&mutex);
    
    return 0;
}


int mySock::read(void* conn, void *data, int len)
{
    int xlen = (len > datalen) ? datalen : len;

    htimed_mutex_lock(&mutex);
    memcpy(data, buffer, xlen);
    datalen -= xlen;
    htimed_mutex_unlock(&mutex);

    return xlen;
}


int mySock::write(void* conn, void* data, int len)
{
    hv::TcpClient tc;
    return tc.send(data, len);
}