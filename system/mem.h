#ifndef __MEM_Hx__
#define __MEM_Hx__

#include "types.h"

typedef enum {
    MEM_SRAM=0,
    MEM_SDRAM,
}MEM_WHERE;


int mem_init(void);
int mem_deinit(void);

void* mem_malloc(MEM_WHERE where, int len, U8 zero);
int mem_free(void *ptr);
int mem_test(void);

#ifdef _WIN32
#define iMalloc(x)         malloc(x)
#define iCalloc(x)         calloc(1,x)

//e means external sdram malloc/calloc first
#define eMalloc(x)         malloc(x)
#define eCalloc(x)         calloc(1,x)

#define xFree(x)           free(x)
#else
//i means internal sram malloc/calloc first
#define iMalloc(x)         mem_malloc(MEM_SRAM,x,0)
#define iCalloc(x)         mem_malloc(MEM_SRAM,x,1)

//e means external sdram malloc/calloc first
#define eMalloc(x)         mem_malloc(MEM_SDRAM,x,0)
#define eCalloc(x)         mem_malloc(MEM_SDRAM,x,1)

#define xFree(x)           mem_free(x)
#endif

#endif

