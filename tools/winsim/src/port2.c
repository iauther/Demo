#include "libserialport.h"
#include "port.h"


//#pragma comment(lib,".\\lib\\libserialport.lib")

#define CHECK(r)    if(r!=SP_OK) return -1;

static int isOpened = 0;
static sp_port_t* sPort = NULL;


int port_init(void)
{
    return 0;
}


int port_free(void)
{
    return 0;
}


int port_open(int port)
{
    char tmp[50];
    sp_return_t r;
    sp_config_t* cfg = NULL;

    CHECK(sp_new_config(&cfg));

    sprintf(tmp, "\\\\.\\COM%d", port);
    CHECK(sp_get_port_by_name(tmp, &sPort));

    CHECK(sp_open(sPort, SP_MODE_READ_WRITE));

    CHECK(sp_get_config(sPort, cfg));

    sp_set_config_baudrate(cfg, 115200);
    sp_set_config_bits(cfg, 8);
    sp_set_config_parity(cfg, SP_PARITY_NONE);
    sp_set_config_stopbits(cfg, 1);
    //sp_set_config_rts(cfg, SP_RTS_OFF);
    //sp_set_config_cts(cfg, SP_CTS_OFF);
    //sp_set_config_dtr(cfg, SP_DTR_OFF);
    //sp_set_config_dsr(cfg, SP_DSR_IGNORE);
    //sp_set_config_xon_xoff(cfg, SP_XONXOFF_DISABLED);
    //sp_set_config_flowcontrol(cfg, SP_FLOWCONTROL_NONE);
    CHECK(sp_set_config(sPort, cfg));
    CHECK(sp_flush(sPort, SP_BUF_BOTH));

    sp_free_config(cfg);
    isOpened = 1;

    return 0;
}

int port_is_opened(void)
{
    return isOpened;
}

int port_close(void)
{
    int r = sp_close(sPort);
    isOpened = 0;

    return r;
}

int port_send(void* data, U16 len)
{
    int r;

    r = sp_blocking_write(sPort, data, len, 0);
    return (r == len) ? 0 : 1;

    //r = sp_nonblocking_write(sPort, data, len);
    //return (r>0) ? 0 : 1;
}


int port_recv(void* data, U16 len)
{
    return sp_blocking_read(sPort, data, len, 0);
}

