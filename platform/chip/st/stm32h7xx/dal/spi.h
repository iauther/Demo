#ifndef __SPI_Hx__
#define __SPI_Hx__

#include "types.h"

typedef enum {
    SPI_1=0,
    SPI_2,
    SPI_3,
    SPI_4,
    SPI_5,
    SPI_6,

    SPI_MAX
}eSPI;

int spi_init(U8 spi, SPI_InitTypeDef *init);

int spi_deinit(U8 spi);

int spi_read(U8 spi, U8 *data, U16 len, U32 timeout);

int spi_write(U8 spi, U8 *data, U16 len, U32 timeout);

int spi_readwrite(U8 spi, U8 *wdata, U8 *rdata, U16 len, U32 timeout);
    
#endif
