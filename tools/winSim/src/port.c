#include <windows.h>
#include "serial.h"
#include "data.h"


static handle_t portHandle=NULL;

int port_init(void)
{
    portHandle = serial_init();
}


int port_free(void)
{
    serial_free(&portHandle);
}


int dev_open(int port)
{
    return  serial_open(portHandle, port, 115200, 0);
}

int dev_is_opened(void)
{
    return  serial_is_pened(portHandle);
}

int dev_close(void)
{
    return serial_close(portHandle);
}

int dev_send(void* data, U16 len)
{
    return serial_write(portHandle, data, len);
}


int dev_recv(void* data, U16 len)
{
    return serial_read(portHandle, data, len);
}

