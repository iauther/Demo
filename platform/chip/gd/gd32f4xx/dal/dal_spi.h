#ifndef __DAL_SPI_Hx__
#define __DAL_SPI_Hx__

#include "types.h"

enum {
    SPI_0=0,
    SPI_1,
    SPI_2,
    SPI_3,
    SPI_4,
    SPI_5,

    SPI_MAX
};

typedef struct {
    buf_t   rx;
    buf_t   tx;
    buf_t   vx;
    buf_t   ox; //dac output
}spi_buf_t;

typedef void (*spi_callback_t)(U16 *rx, U16 *ox, F32 *vx, int cnt);


typedef struct {
    U8              port;
    U8              mast;
    U8              mode;
    U8              bits;
    U8              isMsb;
    U8              softCs;
    U8              quad;
    U8              useDMA;
    spi_callback_t  callback;
    
    spi_buf_t       buf;
    
    handle_t        hdac;
}dal_spi_cfg_t;


handle_t dal_spi_init(dal_spi_cfg_t *cfg);
int dal_spi_deinit(handle_t h);
int dal_spi_read(handle_t h, U16 *data, U16 cnt, U32 timeout);
int dal_spi_write(handle_t h, U16 *data, U16 cnt, U32 timeout);
int dal_spi_readwrite(handle_t h, U16 *wdata, U16 *rdata, U16 cnt, U32 timeout);
    
#endif
