#include "xmem.h"
#include "rtx_mem.h"
#include <stdlib.h>

typedef struct {
    void *start;
    int  len; 
}xmem_para_t;


static xmem_para_t xParam={NULL,0};
int xmem_init(void *start, int len)
{
    int r;
    
    r = rtx_mem_init(start, len);
    if(r) {
        return -1;
    }
    
    xParam.start = start;
    xParam.len = len;
    
    return 0;
}



void* xmem_malloc_align(STRATEGY stg, int len, U8 zero, int alignbytes)
{
    void*  p1;
    void** p2;
    int offset = alignbytes - 1 + sizeof(void*);
    
    if(!xParam.start || !xParam.len) {
        return NULL;
    }
    
    if(SRAM_FIRST) {
        p1 = malloc(len + offset);
        if(!p1) {
            p1 = rtx_mem_alloc(xParam.start, len + offset, 0);
            if (!p1) return NULL;
        }
    }
    else {
        p1 = rtx_mem_alloc(xParam.start, len + offset, 0);
        if (!p1) {
            p1 = malloc(len);
            if (!p1) return NULL;
        }
    }
    
    if(zero) memset(p1, 0, len);

    p2 = (void**)(((size_t)p1 + offset) & ~(alignbytes-1));
    p2[-1] = p1;

    return p2;
}



int xmem_free_align(void *ptr)
{
    int r=0;
    void* p1 = ((void**)ptr)[-1];
    
    if(!xParam.start || !xParam.len || !ptr) {
        return -1;
    }
    
    if(p1+4>=xParam.start) {
        r = rtx_mem_free(xParam.start, p1);
    }
    else {
        free(p1);
    }
    
    return r;
}



int xmem_test(void)
{
    
   
    
    return 0;
}




