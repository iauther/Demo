#ifndef __XMEM_Hx__
#define __XMEM_Hx__

#include "types.h"


handle_t xmem_init(void *start, int len);
int xmem_deinit(handle_t h);
void* xmem_malloc(handle_t h, int size, int zero);
void* xmem_malloc_align(handle_t h, int size, int zero, int alignment);
int xmem_free(handle_t h, void *ptr);
int xmem_free_align(handle_t h, void *ptr);


#define xMalloc         xmem_malloc
#define xFree           xmem_free

#define xAlignMalloc    xmem_malloc_align
#define xAlignFree      xmem_free_align

#endif

