#ifndef __SPIM_Hx__
#define __SPIM_Hx__

#include "types.h"
#include "platform.h"

typedef struct {
    hal_spi_master_port_t           port;
    hal_spi_master_macro_select_t   group;
}spim_info_t;

typedef struct {
    hal_gpio_pin_t  cs;
    hal_gpio_pin_t  clk;
    hal_gpio_pin_t  miso;
    hal_gpio_pin_t  mosi;
}spim_pin_t;

typedef struct {
    U32                                     freq;
    hal_spi_master_bit_order_t              lsb;
    hal_spi_master_clock_polarity_t         cpol;
    hal_spi_master_clock_phase_t            cpha;
    
    hal_spi_master_slave_port_t             sport;         //0: cs 0            1: cs 1
    hal_spi_master_byte_order_t             endian;        //0: little endian   1: big endian
    hal_spi_master_chip_select_polarity_t   cs_polarity;   //0: low select      1: high select
    hal_spi_master_get_tick_mode_t          delay_tick;    //0~7: spi clock tick delay
    hal_spi_master_sample_select_t          sample_edge;   //0: positive edge   1: negative edge
}spim_para_t;

typedef struct {
    spim_pin_t      pin;
    spim_para_t     para;
    U8              useDMA;
}spim_cfg_t;


handle_t spim_init(spim_cfg_t *cfg);

int spim_deinit(handle_t *h);

int spim_get_port(spim_pin_t *pin, hal_spi_master_port_t *port);
    
int spim_read(handle_t h, U8 *data, U32 len);

int spim_write(handle_t h, U8 *data, U32 len);

#endif

