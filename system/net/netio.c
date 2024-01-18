#include "netio.h"
#include "ecxxx.h"
#include "dal_spi.h"
#include "dal_uart.h"
#include "task.h"


typedef struct {
    U8   io;
    union {
         ecxxx_cfg_t    ec;
         dal_uart_cfg_t uc;
         dal_spi_cfg_t  sc;
         //dal_usb_cfg_t  xc;
    }para;
    
    handle_t h;
}netio_handle_t;


handle_t netio_init(netio_cfg_t *cfg)
{
    netio_handle_t *nh=NULL;
    
    if(!cfg || (cfg->io>=NETIO_MAX) || !cfg->para) {
        return NULL;
    }
    
    nh = calloc(1, sizeof(netio_handle_t));
    if(!nh) {
        return NULL;
    }
    nh->io = cfg->io;
    
    switch(nh->io) {
        
        case NETIO_SPI:
        {
            
        }
        break;
        
        case NETIO_USB:
        {
            
        }
        break;
        
        case NETIO_ETH:
        {
            
        }
        break;
        
        case NETIO_UART:
        {
            
        }
        break;
        
        case NETIO_ECXXX:
        {
            nh->para.ec = *(ecxxx_cfg_t*)(cfg->para);
            //nh->h = ecxxx_init(&nh->para.ec);
        }
        break;
    }
    
    if(!nh->h) {
        free(nh);
        return NULL;
    }
    
    return nh;
}


int netio_deinit(handle_t hio)
{
    netio_handle_t *nh=(netio_handle_t*)hio;
    
    if(!nh) {
        return -1;
    }
    
    switch(nh->io) {
        
        case NETIO_SPI:
        {
            dal_spi_deinit(nh->h);
        }
        break;
        
        case NETIO_USB:
        {
            
        }
        break;
        
        case NETIO_ETH:
        {
            
        }
        break;
        
        case NETIO_UART:
        {
            dal_uart_deinit(nh->h);
        }
        break;
        
        case NETIO_ECXXX:
        {
            //ecxxx_deinit(nh->h);
        }
        break;
    }
    
    return 0;
}


int netio_read(handle_t hio, void *data, int len)
{
    return 0;
}


int netio_write(handle_t hio, void *data, int len)
{
    int r=-1;
    netio_handle_t *nh=(netio_handle_t*)hio;
    
    if(!nh) {
        return -1;
    }
    
    switch(nh->io) {
        
        case NETIO_SPI:
        {
            //r = dal_spi_write(nh->h, data, len, 500);
        }
        break;
        
        case NETIO_USB:
        {
            
        }
        break;
        
        case NETIO_ETH:
        {
            
        }
        break;
        
        case NETIO_UART:
        {
            r = dal_uart_write(nh->h, data, len);
        }
        break;
        
        case NETIO_ECXXX:
        {
            //r = ecxxx_write(nh->h, data, len, 500);
        }
        break;
    }
    
    return r;
}


