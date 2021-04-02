#include <string.h>
#include "drv/flash.h"

#define FLASH_CACHE_SIZE 1000

#ifdef STM32F412Zx
#define SECTOR_MAX  24
static U32 sector_size[SECTOR_MAX]={
    16*1024,
    16*1024,
    16*1024,
    16*1024,
    64*1024,
    
    128*1024,
    128*1024,
    128*1024,
    128*1024,
    128*1024,
    128*1024,
    128*1024,
    
    16*1024,
    16*1024,
    16*1024,
    16*1024,
    64*1024,
    
    128*1024,
    128*1024,
    128*1024,
    128*1024,
    128*1024,
    128*1024,
    128*1024, 
};
#elif defined STM32F412Rx
#define SECTOR_MAX  8
static U32 sector_size[SECTOR_MAX]={
    16*1024,
    16*1024,
    16*1024,
    16*1024,
    64*1024,
    
    128*1024,
    128*1024,
    128*1024,
};

#else    //STM32F103xE
#define SECTOR_MAX  12
static U32 sector_size[SECTOR_MAX]={
    16*1024,
    16*1024,
    16*1024,
    16*1024,
    
    64*1024,
    
    128*1024,
    128*1024,
    128*1024,
    128*1024,
    128*1024,
    128*1024,
    128*1024,
};
#endif

static U32 get_sector_addr(int sector)
{
    int i;
    U32 addr=0;
    if(sector>=SECTOR_MAX) {
        return 0xffffffff;
    }
    
    for(i=1; i<sector; i++) {
        addr += sector_size[i-1];
    }
    
    return addr;
}

static int get_sector(U32 addr)
{
    int i;
    U32 s0=0,s1=0;
    
    for(i=0; i<SECTOR_MAX; i++) {
        
        s0 += sector_size[i];
        if(addr>=s0 && addr<s1) {
            return i;
        }
        s0 = s1;
    }
    
    return -1;
}

static int __read_data(U32 offset, U8 *data, U32 len)
{
    U32 i=0;
    U32 ra=FLASH_BASE+offset;

    while(i<len){
        data[i] = *(__IO U8*)(ra+i);
        i++;
    }
    
    return 0;
}




static int __write_data(U32 offset, U8 *data, U32 len)
{
    int err=0,n=0;
    U32 i=0,addr=FLASH_BASE+offset;

    HAL_FLASH_Unlock();
	for(i=0; i<len;) {
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr+i, data[i])!= HAL_OK) {
            ;
            if(++n>3) {
                err = -1;
                break;
            }
			continue;
		}
        else {
            i++;
        }
	}
    HAL_FLASH_Lock();

    return err;
}


static int __erase_sector(int sec)
{
    int r,i;
	uint32_t sectorError = 0;
	FLASH_EraseInitTypeDef pEraseInit;
	
	pEraseInit.TypeErase = TYPEERASE_SECTORS;
	pEraseInit.Banks = FLASH_BANK_1;
	pEraseInit.Sector = sec;
    pEraseInit.NbSectors = 1;
    pEraseInit.VoltageRange = VOLTAGE_RANGE_3;
	
    HAL_FLASH_Unlock();
    for(i=0; i<3; i++) {
        r = HAL_FLASHEx_Erase(&pEraseInit, &sectorError);
        if(r==HAL_OK) {
            break;
        }
    }
    HAL_FLASH_Lock();

    return r;
}

static void __reset_cache(void)
{
    __HAL_FLASH_DATA_CACHE_DISABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();

    __HAL_FLASH_DATA_CACHE_RESET();
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();

    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();
}

int flash_read(U32 offset, U8 *data, U32 len)
{
    int r;
    
    r = __read_data(offset, data, len);
    return r;
}



int flash_write(U32 offset, U8 *data, U32 len)
{
    U32 dlen,wlen=0,ss;
    int r,sec=0,remain=len;

    if(!data || !len) {
        return -1;
    }
    
    r = flash_erase(offset, offset+len-1);
    if(r!=HAL_OK) {
        return -1;
    }
    __reset_cache();
    
    sec = get_sector(offset);
    while(remain>0) {
        ss = sector_size[sec];
        dlen = ((len - wlen)>=ss)?ss:remain;
        
        __write_data(offset+wlen, data+wlen, dlen);
        wlen += dlen;

        remain = len - wlen;
        sec++;
    }

    return 0;
}


int flash_erase(U32 from, U32 to)
{
    int i,r=-1;
    int s1=get_sector(from);
    int s2=get_sector(to);
    for(i=s1; i<=s2; i++) {
        r = __erase_sector(i);
        if(r!=HAL_OK) {
            return -1;
        }
    }

    return 0;
}

