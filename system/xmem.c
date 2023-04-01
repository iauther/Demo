#include "xmem.h"
#include "dal/sdram.h"
#include "rtx_mem.h"

uint64_t sdramBuffer[RTX_MEM_SIZE/8] __attribute__ ((at(SDRAM_ADDR)));


void xmem_init(void)
{
    sdram_init();
    rtx_mem_init(sdramBuffer, RTX_MEM_SIZE);
}


void* xmem_malloc(int size, int zero)
{
    void*  p1;
    
    p1 = (void*)rtx_mem_alloc(sdramBuffer, size, 0);
    if(p1 && zero>0) {
        memset(p1, 0, size);
    }
    
    return p1;
}


void* mxem_malloc_align(int size, int zero, int alignment)
{
    void*  p1;
    void** p2;
    int offset = alignment - 1 + sizeof(void*);

    p1 = xmem_malloc(size + offset, zero);
    if (p1 == NULL)
        return NULL;

    p2 = (void**)(((size_t)p1 + offset) & ~(alignment-1));
    p2[-1] = p1;

    return p2;
}


void xmem_free(void *ptr)
{
    rtx_mem_free(sdramBuffer, ptr);
}


void xmem_free_align(void *ptr)
{
    void* p1 = ((void**)ptr)[-1];
    rtx_mem_free(sdramBuffer, p1);
}



