#ifndef __XMEM_Hx__
#define __XMEM_Hx__

#include "types.h"

typedef enum {
    SRAM_FIRST=0,
    SDRAM_FIRST,
}STRATEGY;


int xmem_init(void *start, int len);
void* xmem_malloc_align(STRATEGY stg, int len, U8 zero, int alignbytes);
int xmem_free_align(void *ptr);
int xmem_test(void);

//i means internal sram malloc/calloc first
#define iMalloc(x)         xmem_malloc_align(SRAM_FIRST,x,0,4)
#define iCalloc(x)         xmem_malloc_align(SRAM_FIRST,x,1,4)
#define iMallocA(x,y)      xmem_malloc_align(SRAM_FIRST,x,0,y)
#define iCallocA(x,y)      xmem_malloc_align(SRAM_FIRST,x,1,y)

//e means external sdram malloc/calloc first
#define eMalloc(x)         xmem_malloc_align(SDRAM_FIRST,x,0,4)
#define eCalloc(x)         xmem_malloc_align(SDRAM_FIRST,x,1,4)
#define eMallocA(x,y)      xmem_malloc_align(SDRAM_FIRST,x,0,y)
#define eCallocA(x,y)      xmem_malloc_align(SDRAM_FIRST,x,1,y)

#define xFree(x)           xmem_free_align(x)





#endif

