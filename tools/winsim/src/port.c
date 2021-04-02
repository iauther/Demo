#include "serial.h"
#include "port.h"


static handle_t portHandle=NULL;

int port_init(void)
{
    portHandle = serial_init();
    return portHandle ? 0 : -1;
}


int port_free(void)
{
    serial_free(&portHandle);
    portHandle = NULL;
    return 0;
}


int port_open(int port)
{
    return  serial_open(portHandle, port, 115200, 0);
}

int port_is_opened(void)
{
    return  serial_is_opened(portHandle);
}

int port_close(void)
{
    return serial_close(portHandle);
}

int port_send(void* data, U16 len)
{
    return serial_write(portHandle, data, len);
}


int port_recv(void* data, U16 len)
{
    return serial_read(portHandle, data, len);
}

