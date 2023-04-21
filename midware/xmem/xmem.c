#include "xmem.h"
#include "rtx_mem.h"


typedef struct {
    void *start;
    int  len; 
}xmem_handle_t;


handle_t xmem_init(void *start, int len)
{
    int r;
    xmem_handle_t *h;
    
    h = calloc(1, sizeof(xmem_handle_t));
    if(!h) {
        return NULL;
    }
    
    r = rtx_mem_init(start, len);
    if(r) {
        free(h);
        return NULL;
    }
    
    h->start = start;
    h->len = len;
    
    return h;
}


int xmem_deinit(handle_t h)
{
    xmem_handle_t *xh=(xmem_handle_t*)h;
    
    if(!h) {
        return -1;
    }
    free(h);
    
    return 0;
}


void* xmem_malloc(handle_t h, int size, int zero)
{
    void*  p1;
    xmem_handle_t *xh=(xmem_handle_t*)h;
    
    if(!xh || !size) {
        return NULL;
    }
    
    p1 = (void*)rtx_mem_alloc(xh->start, size, 0);
    if(p1 && zero>0) {
        memset(p1, 0, size);
    }
    
    return p1;
}


void* mxem_malloc_align(handle_t h, int size, int zero, int alignment)
{
    void*  p1;
    void** p2;
    xmem_handle_t *xh=(xmem_handle_t*)h;
    int offset = alignment - 1 + sizeof(void*);

    p1 = xmem_malloc(h, size + offset, zero);
    if (p1 == NULL)
        return NULL;

    p2 = (void**)(((size_t)p1 + offset) & ~(alignment-1));
    p2[-1] = p1;

    return p2;
}


int xmem_free(handle_t h, void *ptr)
{
    xmem_handle_t *xh=(xmem_handle_t*)h;
    if(!xh || !ptr) {
        return -1;
    }
    
    return rtx_mem_free(xh->start, ptr);
}


int xmem_free_align(handle_t h, void *ptr)
{
    void* p1 = ((void**)ptr)[-1];
    xmem_handle_t *xh=(xmem_handle_t*)h;
    
    if(!xh || !ptr) {
        return -1;
    }
    
    return rtx_mem_free(xh->start, p1);
}



