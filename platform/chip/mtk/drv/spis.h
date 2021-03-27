#ifndef __SPIS_Hx__
#define __SPIS_Hx__

#include "types.h"

typedef int (*spis_callback)(int rw, int status, U8 *data, U32 len);        //0: read   1: write


typedef enum {
    SPIS_RECV,
    SPIS_SEND,  
}SPIS_STAT;

typedef struct {
    hal_spi_slave_port_t           port;
}spis_info_t;

typedef struct {
    hal_gpio_pin_t  cs;
    hal_gpio_pin_t  clk;
    hal_gpio_pin_t  miso;
    hal_gpio_pin_t  mosi;
}spis_pin_t;

typedef struct {
    hal_spi_slave_bit_order_t       lsb;
    hal_spi_slave_clock_polarity_t  cpol;
    hal_spi_slave_clock_phase_t     cpha;
    U32                             timeout;      //0~7: spi clock tick delay
}spis_para_t;

typedef struct {
    spis_callback   callback;
    void            *user_data;
}spis_callback_t;

typedef struct {
    spis_pin_t      pin;
    spis_para_t     para;
    spis_callback_t cb;
}spis_cfg_t;


int spis_init(spis_cfg_t *cfg, spis_info_t *info);

int spis_set_callback(hal_spi_slave_port_t port, spis_callback_t *cb);

int spis_recv(hal_spi_slave_port_t port, U8 *data, U32 len, U32 *realen);

int spis_send(hal_spi_slave_port_t port, U8 *data, U32 len, U32 *realen);


#endif

