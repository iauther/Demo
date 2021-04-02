#include <windows.h>
#include "serial.h"
#include "data.h"


CSerial mSerial;

int dev_open(int port)
{
    return  mSerial.Open(port, 115200, 0);
}

int dev_is_opened(void)
{
    return  mSerial.isOpened();
}

int dev_close(void)
{
    return mSerial.Close();
}

int dev_send(void* data, U16 len)
{
    return mSerial.Write(data, len);
}


int dev_recv(void* data, U16 len)
{
    return mSerial.Read(data, len);
}

