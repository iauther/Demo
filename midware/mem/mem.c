#include "mem.h"
#include "log.h"
#include "lock.h"
#include "tiny.h"
#include "rtx_mem.h"
#include "dal_sdram.h"
#include <stdlib.h>

typedef struct {
    U8      *start;
    int     len;
    
    handle_t lock;
}mem_para_t;


#define OUT_RANGE(x,a,b) (((U32)(x)<(a)) || ((U32)(x)>(b)))

extern U32 __heap_base,__heap_limit;
#define I_OUT_RANGE(x) OUT_RANGE(x,__heap_base,__heap_limit)
#define E_OUT_RANGE(x) OUT_RANGE(x,SDRAM_ADDR,SDRAM_ADDR+SDRAM_LEN)

//#define USE_TINY


static mem_para_t xParam={(U8*)SDRAM_ADDR,SDRAM_LEN, NULL};

int mem_init(void)
{
    int r;
    
#ifdef USE_TINY
    tiny_init(xParam.start, xParam.len);
#else
    r = rtx_mem_init(xParam.start, xParam.len);
    if(r) {
        LOGE("___ mem_init failed\n");
        return -1;
    }
#endif
    
    xParam.lock = lock_init();
    
    return 0;
}



void* mem_malloc(STRATEGY stg, int len, U8 zero)
{
    void*  p1;
    void** p2;
    
    if(!xParam.start || !xParam.len || len<=0) {
        return NULL;
    }
    
    lock_on(xParam.lock);
    if(stg==SRAM_FIRST) {
        p1 = malloc(len);
        if(!p1) {
            LOGE("___ malloc %d failed, 0x%08x, call tiny_malloc or rtx_mem_alloc\n", len, p1);
            
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
            LOGE("___ tiny_malloc or rtx_mem_alloc %d, failed, 0x%08x\n, call malloc.\n", len, p1);
            
            p1 = malloc(len);
        }
    }
    
    if(((U32)p1)%4) {
        LOGE("___ p1: 0x%08x not align 4 byte!!!\n", (U32)p1);
        lock_off(xParam.lock);
        return NULL;
    }
    
    if(p1 && zero) memset(p1, 0, len);
    lock_off(xParam.lock);

    return p1;
}



int mem_free(void *ptr)
{
    int r=0;
    void* p1 = ptr;
    
    
    if(!xParam.start || !xParam.len || !ptr) {
        return -1;
    }
    
    lock_on(xParam.lock);
    
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
    lock_off(xParam.lock);
    
    return r;
}


int mem_get_free(void)
{
    //extern U32 base_sp;
    //return (int)(base_sp - __malloc_heap_start);
    return 0;
}

int mem_get_used(void)
{
    //extern ulong base_sp;
    //return (int)(_heap_base - __malloc_heap_start);
    return 0;
}




int mem_test(void)
{
    
    return 0;
}




