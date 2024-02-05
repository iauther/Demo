#include "mem.h"
#include "log.h"
#include "lock.h"
#include <stdlib.h>
#ifndef _WIN32
#include "tiny.h"
#include "rtx_mem.h"
#include "dal_sdram.h"
#endif

typedef struct {
    U8      *start;
    int     len;
    
    handle_t lock;
}mem_para_t;


//#define USE_TINY



#ifdef _WIN32
static mem_para_t xParam={(U8*)0x2000000,2000000, NULL};
#else
#define OUT_RANGE(x,a,b) (((U32)(x)<(a)) || ((U32)(x)>(b)))

extern U32 __heap_base,__heap_limit,Heap_Size;
#define I_OUT_RANGE(x) OUT_RANGE(x,__heap_base,__heap_limit)
#define E_OUT_RANGE(x) OUT_RANGE(x,SDRAM_ADDR,SDRAM_ADDR+SDRAM_LEN)

static mem_para_t xParam={(U8*)SDRAM_ADDR,SDRAM_LEN, NULL};
#endif



int mem_init(void)
{
    int r;

#ifndef _WIN32
    dal_sdram_init();
#ifdef USE_TINY
    tiny_init(xParam.start, xParam.len);
#else
    r = rtx_mem_init(xParam.start, xParam.len);
    if(r) {
        LOGE("___ mem_init failed\n");
        return -1;
    }
#endif
    
#endif
    
    xParam.lock = lock_init();
    
    return 0;
}


int mem_deinit(void)
{
    int r;

#ifndef _WIN32
    dal_sdram_deinit();
#endif
    
     lock_free(xParam.lock);
    
    return 0;
}



void* mem_malloc(MEM_WHERE where, int len, U8 zero)
{
    void*  p1=NULL;
    
    if(!xParam.start || !xParam.len || len<=0) {
        return NULL;
    }
    
    lock_on(xParam.lock);
#ifdef _WIN32
    p1 = malloc(len);
#else
    if(where==MEM_SRAM) {
        p1 = malloc(len);
        
    }
    else {
    #ifdef USE_TINY
        p1 = tiny_malloc(len);
    #else
        p1 = rtx_mem_alloc(xParam.start, len);
    #endif
    }
#endif
    
    if(p1) {
        if(zero) {
            memset(p1, 0, len);
        }
    }
    else {
        LOGE("mem_malloc failed\n");
    }
    lock_off(xParam.lock);
    
    return p1;
}



int mem_free(void *ptr)
{
    int r=0;
    void* p1 = ptr;
    
    
    if(!xParam.start || !xParam.len || !p1) {
        return -1;
    }
    
    lock_on(xParam.lock);
#ifdef _WIN32
    free(p1);
#else
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
#endif
    
    if(r==-1) {
        LOGE("mem free failed\n");
    }
    
    lock_off(xParam.lock);
    
    return r;
}


U32 mem_used(MEM_WHERE where)
{
    U32 used=0;
    
    if(where==MEM_SRAM) {
        extern U32 __heap_base,__heap_limit,Heap_Size;
        //return (U32)(base_sp - __malloc_heap_start);
    }
    else {
#ifdef USE_TINY
        tiny_summary summ = tiny_inspect();
        used = summ.sections.total-summ.sections.free;
#else
        used = rtx_mem_used(xParam.start);
#endif
    }
    
    return used;
}

U32 mem_unused(MEM_WHERE where)
{
    U32 unused=0;
    
    if(where==MEM_SRAM) {
        extern U32 __heap_base,__heap_limit,Heap_Size;
        //return (int)(base_sp - __malloc_heap_start);
    }
    else {
#ifdef USE_TINY
        tiny_summary summ = tiny_inspect();
        unused = summ.sections.free;
#else
        unused = rtx_mem_unused(xParam.start);
#endif
    }
    
    return unused;
}




int mem_test(void)
{
    
    return 0;
}




