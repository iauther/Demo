#include "gd32f4xx.h"
#include "dal_nor.h"
#include "log.h"

#define DATA(addr)      (*((__IO U8*)(addr)))

#define PAGE(addr)      ((addr)/NOR_PAGE_SIZE)
#define COUNT(addr)     ((addr)%NOR_PAGE_SIZE)


static U32 get_sector(U32 addr)
{
    U32 xaddr=FLASH_BASE|addr;
    
	if(xaddr<ADDR_FLASH_SECTOR_1)			    return FLASH_Sector_0;
	else if(xaddr<ADDR_FLASH_SECTOR_2)			return FLASH_Sector_1;
	else if(xaddr<ADDR_FLASH_SECTOR_3)			return FLASH_Sector_2;
	else if(xaddr<ADDR_FLASH_SECTOR_4)			return FLASH_Sector_3;
	else if(xaddr<ADDR_FLASH_SECTOR_5)			return FLASH_Sector_4;
	else if(xaddr<ADDR_FLASH_SECTOR_6)			return FLASH_Sector_5;
	else if(xaddr<ADDR_FLASH_SECTOR_7)			return FLASH_Sector_6;
	else if(xaddr<ADDR_FLASH_SECTOR_8)			return FLASH_Sector_7;
	else if(xaddr<ADDR_FLASH_SECTOR_9)			return FLASH_Sector_8;
	else if(xaddr<ADDR_FLASH_SECTOR_10)		    return FLASH_Sector_9;
	else if(xaddr<ADDR_FLASH_SECTOR_11)		    return FLASH_Sector_10;
    
	return FLASH_Sector_11;	
}

static int nor_read(U32 addr, void *data, U32 len)
{
    U8 *p8=(U8*)data;
    U32 i,xaddr=addr|FLASH_BASE;
    
    for(i=0; i<len; i++){
        p8[i] = DATA(xaddr+i);
    }
    
    return 0;
}
static int nor_check(U32 addr, void *data, U32 len)
{
    int r=0;
    U8 *p=(U8*)data,*tmp;
    U32 i,cnt,rem,tlen=0;
    U32 ulen=NOR_PAGE_SIZE;
    
    tmp = (U8*)malloc(ulen);
    if(!tmp) {
        return -1;
    }
    cnt = len/ulen;
    rem = len%ulen;

    for(i=0; i<cnt; i++) {
        nor_read(addr+tlen, tmp, ulen);
        if(memcmp(p+tlen, tmp, ulen)) {
            LOGE("___ nor_check failed 1, addr: 0x%x, len: %d\n", addr+tlen, ulen);
            r = -1;
            break;
        }
        tlen += ulen;
    }
    if(r==0 && rem) {
        nor_read(addr+tlen, tmp, rem);
        if(memcmp(p+tlen, tmp, rem)) {
            LOGE("___ nor_check failed 2, addr: 0x%x, len: %d\n", addr+tlen, rem);
            r = -1; 
        }
    }
    free(tmp);
    
    return r;
}
static int nor_write(U32 addr, void *data, U32 len, U8 chk)
{
    int r=0;
    U8 *p8=(U8*)data;
    fmc_state_enum st;
    U32 i,xaddr=addr|FLASH_BASE;
    
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    for(i=0; i<len; i++){
        st = fmc_byte_program(xaddr+i, p8[i]);
        if(st != FMC_READY){
            LOGE("nor_write, fmc_byte_program 0x%08x failed %d\n", addr+i, st);
            r = -1; break;
        }
    }
    fmc_lock();
    
    if(r==0 && chk) {
        r = nor_check(addr, data, len);
    }
    
    return r;
}
static int is_erased(U32 addr, U32 len)
{
    U32 i,xaddr=addr|FLASH_BASE;
    for(i=0; i<len; i++){
        if(DATA(xaddr)!=0xff) {
            return 0;
        }
    }
    
    return 1;
}

static int is_page_erased(U32 page)
{
    U32 addr=page*NOR_PAGE_SIZE;
    
    return is_erased(addr, NOR_PAGE_SIZE);
}

static int page_erase(U32 page)
{
    int r=0;
    fmc_state_enum st;
    U32 addr=(page*NOR_PAGE_SIZE)|FLASH_BASE;
    
    if(is_page_erased(page)) {
        return 0;
    }
    
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_OPERR | FMC_FLAG_WPERR | FMC_FLAG_PGMERR | FMC_FLAG_PGSERR);
    st = fmc_page_erase(addr);
    if(st != FMC_READY){
        LOGE("___ page_erase failed, page: %d, err: %d\n", page, st);
        r = -1;
    }
    fmc_lock();
    
    return r;
}
static int page_read(U32 page, void *data, U32 len)
{
    U32 addr=(page*NOR_PAGE_SIZE);
    
    return nor_read(addr, data, len);
}
static int page_write(U32 page, void *data, U32 len, U8 chk)
{
    int r=-1;
    U32 addr=(page*NOR_PAGE_SIZE);
    
    r = page_erase(page);
    if(r) {
        return r;
    }
    
    return nor_write(addr, data, len, chk);
}
static int nor_erase(U32 addr, U32 len)
{
    int r=0;
    U32 i,s,s1,e,e1;
    U32 ulen=NOR_PAGE_SIZE;
    
    s  = PAGE(addr);     s1 = COUNT(addr);
    e  = PAGE(addr+len); e1 = COUNT(addr+len);
    if(s1) {
        U8 *p=(U8*)malloc(s1);
        if(!p) {
            LOGE("___ nor_erase 1, malloc %d failed\n", s1);
            return -1;
        }
        page_read(s, p, s1); 
        r = page_write(s, p, s1, 1);
        free(p);
        
        if(r) {
            return -1;
        }
        
        s++;
    }
    
    for(i=s; i<e; i++) {
        r = page_erase(i);
        if(r) {
            return -1;
        }
    }
    
    if((s1==0 || e>s) && e1) {
        U8 *p=(U8*)malloc(ulen-e1);
        if(!p) {
            LOGE("___ nor_erase 2, malloc %d failed\n", e1);
            return -1;
        }
        nor_read(addr+len, p, ulen-e1);
        r = page_erase(e);
        if(r==0) {
            r = nor_write(addr+len, p, ulen-e1, 1);
        }
        free(p);
        
        if(r) {
            return -1;
        }
    }
    
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
    return nor_erase(addr, len);
}



int dal_nor_read(U32 addr, void *data, U32 len)
{
    return nor_read(addr, data, len);
}


int dal_nor_write(U32 addr, void *data, U32 len, U8 erase, U8 chk)
{
    int r=0;
    
    if(erase) {
        r = nor_erase(addr, len);
        if(r) {
            return -1;
        }
    }
    
    r = nor_write(addr, data, len, chk);
    
    return r;
}


#include "cfg.h"
int dal_nor_test(void)
{
    #define TCNT        10000
    #define TLEN        (TCNT*1)
    #define ONELEN      1800
    int i,r;
    U32 tlen=0,xlen;
    U8 *pbuf=(U8*)malloc(TLEN);
    
    U32 addr=APP_OFFSET;
    if(pbuf) {
        
        for(i=0; i<TCNT; i++) {
            pbuf[i] = 77;
        }
        
        while(1) {
            
            if(tlen>=TLEN) {
                break;
            }
            
            if((tlen+ONELEN)>TLEN) {
                xlen = TLEN-tlen;
            }
            else {
                xlen = ONELEN;
            }
            
            r = dal_nor_write(addr+tlen, pbuf+tlen, xlen, 1, 1);
            if(r) {
                LOGE("____ dal_nor_write failed, %d, %d\n", tlen, xlen);
                break;
            }
            tlen += xlen;
        }
        
        if(r==0) {
            memset(pbuf, 0, TLEN);
            dal_nor_read(addr,  pbuf, TLEN);
        }
        
        free(pbuf);
    }
    
    return 0;
}




