#ifndef __MEM_Hx__
#define __MEM_Hx__

#include "types.h"

typedef enum {
    SRAM_FIRST=0,
    SDRAM_FIRST,
}STRATEGY;


int mem_init(void);
int mem_deinit(void);

void* mem_malloc(STRATEGY stg, int len, U8 zero);
int mem_free(void *ptr);
int mem_test(void);

//i means internal sram malloc/calloc first
#define iMalloc(x)         mem_malloc(SRAM_FIRST,x,0)
#define iCalloc(x)         mem_malloc(SRAM_FIRST,x,1)

//e means external sdram malloc/calloc first
#define eMalloc(x)         mem_malloc(SDRAM_FIRST,x,0)
#define eCalloc(x)         mem_malloc(SDRAM_FIRST,x,1)

#define xFree(x)           mem_free(x)

#endif

