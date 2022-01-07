#ifndef __SPI_Hx__
#define __SPI_Hx__

#include "types.h"

typedef enum {
    SPI_1,
    SPI_2,
    SPI_3,

    SPI_MAX
}eSPI;

int spi_init(eSPI spi, SPI_InitTypeDef *init);

int spi_deinit(eSPI spi);

int spi_read(eSPI spi, U8 *data, U16 len, U32 timeout);

int spi_write(eSPI spi, U8 *data, U16 len, U32 timeout);

int spi_readwrite(eSPI spi, U8 *wdata, U8 *rdata, U16 len, U32 timeout);
    
#endif
