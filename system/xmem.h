#ifndef __XMEM_Hx__
#define __XMEM_Hx__

#include "types.h"


void xmem_init(void);
void* xmem_malloc(int size, int zero);
void* xmem_malloc_align(int size, int zero, int alignment);
void xmem_free(void *ptr);
void xmem_free_align(void *ptr);


#define xMalloc         xmem_malloc
#define xFree           xmem_free

#define xAlignMalloc    xmem_malloc_align
#define xAlognFree      xmem_free_align

#endif

