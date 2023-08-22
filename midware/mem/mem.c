#include "mem.h"
#include "lock.h"
#include "tiny.h"
#include "rtx_mem.h"
#include "dal_sdram.h"
#include <stdlib.h>

typedef struct {
    U8 *start;
    int  len; 
}mem_para_t;


#define USE_TINY


static U8 *sdramAddr=(U8*)SDRAM_ADDR;
static U32 sdramLength=SDRAM_LEN;

static mem_para_t xParam={NULL,0};

int mem_init(void)
{
    int r;
    
    lock_static_hold(LOCK_MEM);
    
#ifdef USE_TINY
    tiny_init(sdramAddr, sdramLength);
#else
    r = rtx_mem_init(sdramBuffer, sdramLength);
    if(r) {
        lock_static_release(LOCK_MEM);
        return -1;
    }
#endif
    
    xParam.start = sdramAddr;
    xParam.len = sdramLength;
    lock_static_release(LOCK_MEM);
    
    return 0;
}



void* mem_malloc(STRATEGY stg, int len, U8 zero)
{
    void*  p1;
    void** p2;
    
    if(!xParam.start || !xParam.len) {
        return NULL;
    }
    
    lock_static_hold(LOCK_MEM);
    if(stg==SRAM_FIRST) {
        p1 = malloc(len);
        if(!p1) {
#ifdef USE_TINY
            p1 = tiny_malloc(len);
#else
            p1 = rtx_mem_alloc(xParam.start, len);
#endif
        }
    }
    else {
#ifdef USE_TINY
        p1 = tiny_malloc(len);
#else
        p1 = rtx_mem_alloc(xParam.start, len);
#endif
        if (!p1) {
            p1 = malloc(len);
        }
    }
    
    if(p1 && zero) memset(p1, 0, len);
    lock_static_release(LOCK_MEM);

    return p1;
}



int mem_free(void *ptr)
{
    int r=0;
    void* p1 = ptr;
    
    lock_static_hold(LOCK_MEM);
    if(!xParam.start || !xParam.len || !ptr) {
        lock_static_release(LOCK_MEM);
        return -1;
    }
    
    if((U32)p1>=(U32)xParam.start) {
#ifdef USE_TINY
        tiny_free(p1);
#else
        r = rtx_mem_free(xParam.start, p1);
#endif
    }
    else {
        free(p1);
    }
    lock_static_release(LOCK_MEM);
    
    return r;
}



int mem_test(void)
{
    
    return 0;
}




