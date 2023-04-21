#ifndef __DAL_DMA_Hx__
#define __DAL_DMA_Hx__

#include "types.h"


typedef void (*spi_func)(SPI_HandleTypeDef *hspi);

typedef struct {
    spi_func        fn;
    U8              *buf;
    U16             blen;
    U16             dlen;       //data length
}dma_info_t;


typedef struct {
    dma_info_t      rx;
    dma_info_t      tx;
}dma_para_t;


int dal_dma_init(DMA_HandleTypeDef *hdma);

int dal_dma_deinit(DMA_HandleTypeDef *hdma);


#define dma_link(handle, hdma)      handle->hdmarx = hdma;\
                                    hdma->Parent = handle

#endif
