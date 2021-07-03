#include "SerialPort.h"
#include "port.h"


#pragma comment(lib, __FILE__"\\..\\..\\lib\\libcserialportd.lib")

#define CHECK(r)    if(r!=SP_OK) return -1;

using namespace itas109;

static int isOpened = 0;
CSerialPort comPort;



int port_init(void)
{
    //
    return 0;
}


int port_free(void)
{
    return 0;
}


int port_open(int port)
{
    bool r;
    char tmp[50];

    sprintf(tmp, "\\\\.\\COM%d", port);
    comPort.init(tmp, itas109::BaudRate115200, itas109::ParityNone, itas109::DataBits8, itas109::StopOne, itas109::FlowNone, 4*1024);
    r = comPort.open();
    comPort.setOperateMode(itas109::SynchronousOperate);
    comPort.setReadTimeInterval(5);

    return 0;
}

int port_is_opened(void)
{
    return comPort.isOpened();
}

int port_close(void)
{
    comPort.close();
    return 0;
}

int port_send(void* data, U16 len)
{
    int r;

    if (comPort.isOpened()) {
        r = comPort.writeData((char*)data, len);
        return (r > 0) ? 0 : 1;
    }
    else {
        return 0;
    }
}


int port_recv(void* data, U16 len)
{
    int r;

    if (comPort.isOpened()) {
        r = comPort.readData((char*)data, len);
        return (r > 0) ? 0 : 1;
    }
    else {
        return 0;
    }

}


