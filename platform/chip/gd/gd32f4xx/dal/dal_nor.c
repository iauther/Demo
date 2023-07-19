#include "gd32f4xx.h"
#include "dal_nor.h"
#include "log.h"


#define NOR_PAGE_BYTES  4096

static U16 get_sector(U32 addr)
{
	if(addr<ADDR_FLASH_SECTOR_1)			    return FLASH_Sector_0;
	else if(addr<ADDR_FLASH_SECTOR_2)			return FLASH_Sector_1;
	else if(addr<ADDR_FLASH_SECTOR_3)			return FLASH_Sector_2;
	else if(addr<ADDR_FLASH_SECTOR_4)			return FLASH_Sector_3;
	else if(addr<ADDR_FLASH_SECTOR_5)			return FLASH_Sector_4;
	else if(addr<ADDR_FLASH_SECTOR_6)			return FLASH_Sector_5;
	else if(addr<ADDR_FLASH_SECTOR_7)			return FLASH_Sector_6;
	else if(addr<ADDR_FLASH_SECTOR_8)			return FLASH_Sector_7;
	else if(addr<ADDR_FLASH_SECTOR_9)			return FLASH_Sector_8;
	else if(addr<ADDR_FLASH_SECTOR_10)		    return FLASH_Sector_9;
	else if(addr<ADDR_FLASH_SECTOR_11)		    return FLASH_Sector_10; 
	return FLASH_Sector_11;	
}
static U16 get_page(U32 addr)
{
    U16 page, count;

    page  = addr/NOR_PAGE_BYTES;
    count = addr%NOR_PAGE_BYTES;
    
    return page+(count?1:0);
}
static int page_erase(U32 page)
{
    fmc_state_enum st;
    U32 addr=FLASH_BASE|(page*NOR_PAGE_BYTES);
    
    st = fmc_page_erase(addr);
    if(st != FMC_READY){
        return st;
    }
    
    return 0;
}

static int page_write(U32 addr, void *data, U32 len)
{
    int r;
    U32 i;
    U8 *p8=(U8*)data;
    fmc_state_enum st;
    U32 xaddr=addr|FLASH_BASE;
    
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    
    for(i=0; i<len; i++){
        st = fmc_byte_program(xaddr+i, p8[i]);
        if(st != FMC_READY){
            LOGE("dal_nor_write, fmc_byte_program 0x%08x failed %d\n", xaddr+i, st);
            r = -1; break;
        }
    }
    fmc_lock();
    
    return 0;
}



///////////////////////////////////////////////////////////////////////

int dal_nor_init(void)
{
    return 0;
}


int dal_nor_deinit(void)
{
    return 0;
}


int dal_nor_erase(U32 addr, U32 len)
{
    int r=0;
    U32 i,s,e;
    fmc_state_enum st;
    U32 xaddr=addr;
    
    LOGD("__dal_nor_erase, 0x%x, len: %d\n", xaddr, len);
    
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    
    s = get_page(xaddr);
    e = get_page(xaddr+len);
    LOGD("__dal_nor_erase page from %d to %d\n", s, e);
    
    for(i = s; i <= e; i++){
        r = page_erase(i);
        if(r){
            LOGE("__dal_nor_erase, fmc_page_erase %d failed %d\n", i, st);
            r = -1; break;
        }
    }
    fmc_lock();
    
    return r;
}



int dal_nor_read(U32 addr, void *data, U32 len)
{
    int i;
    U8 *p8=(U8*)data;
    U32 xaddr=addr|FLASH_BASE;
    
    for(i=0; i<len; i++){
        p8[i] = *(__IO U8*)(xaddr+i);
    }
    
    return 0;
}


static U8 nor_page_tmp[NOR_PAGE_BYTES];
int dal_nor_write(U32 addr, void *data, U32 len, U8 check)
{
    int r=0;
    U32 i;
    U8 *p8=(U8*)data;
    fmc_state_enum st;
    U32 xaddr=addr|FLASH_BASE;
    
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    
    for(i=0; i<len; i++){
        st = fmc_byte_program(xaddr+i, p8[i]);
        if(st != FMC_READY){
            LOGE("dal_nor_write, fmc_byte_program 0x%08x failed %d\n", xaddr+i, st);
            r = -1; break;
        }
    }
    fmc_lock();
    
    if(check) {
        U8 *pb=(U8*)malloc(len);
        
        if(!pb) {
            LOGE("dal_nor_write, read malloc failed\n");
            return -1;
        }
        
        dal_nor_read(addr, pb, len);
        if(memcmp(data, pb, len)) {
            LOGE("dal_nor_write, read check failed\n");
            r = -1;
        }
        free(pb);
    }
    
    return r;
}


#include "cfg.h"
int dal_nor_test(void)
{
    #define TCNT        200000
    #define TLEN        (TCNT*1)
    #define ONELEN      700
    int i,r;
    U32 tlen=0;
    U8 *pbuf=(U8*)malloc(TLEN);
    
    if(pbuf) {
        
        dal_nor_erase(APP_OFFSET, TLEN);
        dal_nor_read(APP_OFFSET, pbuf, TLEN);
        
        for(i=0; i<TCNT; i++) {
            pbuf[i] = 77;
        }
        
        while(1) {
            r = dal_nor_write(APP_OFFSET+tlen, pbuf+tlen, ONELEN, 1);
            if(r) {
                LOGE("____ dal_nor_write failed\n");
                break;
            }
            
            tlen += ONELEN;
            if(tlen>=TLEN) {
                break;
            }
        }
        
        if(r==0) {
            memset(pbuf, 0, TLEN);
            dal_nor_read(APP_OFFSET,  pbuf, TLEN);
        }
        
        free(pbuf);
    }
    
    return 0;
}




