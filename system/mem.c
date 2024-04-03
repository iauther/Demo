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

#ifndef _WIN32
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
#endif

    return used;
}

U32 mem_unused(MEM_WHERE where)
{
    U32 unused=0;

#ifndef _WIN32
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
#endif

    return unused;
}


#ifndef _WIN32
#include "dal_delay.h"
static void mem_ttt(char *p, void *s, void *d, int len, int cnt)
{
    U32 i,t1,t2,t3,x,y;
    
    t1 = dal_get_tick_ms();
    for(i=0; i<cnt; i++) {
        memset(s, 0x88, len);
    }
    t2 = dal_get_tick_ms();
    
    for(i=0; i<cnt; i++) {
        memcpy(s, d, len);
    }
    t3 = dal_get_tick_ms();
    
    x = (t2-t1);
    y = (t3-t2);
    
    LOGD("%s w: %dms\n", p, x);
    LOGD("%s r: %dms\n", p, y);
}
#endif

int mem_test(void)
{
#if 0
    int cnt=1000;
    int len=100*KB;
    char *p1,*p2;
    
    p1=iMalloc(len);
    p2=iMalloc(len);
    mem_ttt("sram->sram", p1, p2, len, cnt);
    xFree(p1);xFree(p2);
    
    p1=eMalloc(len);
    p2=eMalloc(len);
    mem_ttt("sdram->sdram", p1, p2, len, cnt);
    xFree(p1);xFree(p2);
    
    p1=iMalloc(len);
    p2=eMalloc(len);
    mem_ttt("sram->sdram", p1, p2, len, cnt);
    mem_ttt("sdram->sram", p1, p2, len, cnt);
    xFree(p1);xFree(p2);
#endif

    return 0;
}




