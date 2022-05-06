#ifndef __SPI_Hx__
#define __SPI_Hx__

#include "types.h"

typedef enum {
    SPI_1,
    SPI_2,
    SPI_3,

    SPI_MAX
}eSPI;

typedef struct {
    U8              port;
    SPI_InitTypeDef init; 
}spi_cfg_t;



handle_t spi_init(spi_cfg_t *cfg);

int spi_deinit(handle_t *h);

int spi_read(handle_t h, U8 *data, U16 len, U32 timeout);

int spi_write(handle_t h, U8 *data, U16 len, U32 timeout);

int spi_readwrite(handle_t h, U8 *wdata, U8 *rdata, U16 len, U32 timeout);
    
#endif
