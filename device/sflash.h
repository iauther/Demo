#ifndef __SFLASH_Hx__
#define __SFLASH_Hx__

#include "types.h"

/*
    W25Q128JV
    65536 pages of 256B each     65536*256B=16MB
    
    sector size 4KB
    
    Pages erased in groups of 16 (4KB sector erase), 
                    groups of 128 (32KB block erase), 
                    groups of 256 (64KB block erase)
                    entire chip (chip erase)
    
*/

#define SFLASH_SECTOR_CNT          0x1000
#define SFLASH_SECTOR_SIZE         0x1000
#define SFLASH_PAGE_SIZE           0x100
#define SFLASH_BLOCK_SIZE          0x8000

#define SFLASH_FS_SECTOR_OFFSET    0x200

int sflash_init(void);
int sflash_deinit(void);
int sflash_erase_all(void);
int sflash_erase(U32 addr, U32 len);
int sflash_erase_sector(U32 sec);
int sflash_read_sector(U32 sec, void *data);
int sflash_write_sector(U32 sec, void *data);
int sflash_write(U32 addr, void *data, U32 len, U8 erase, U8 chk);
int sflash_read(U32 addr, void *data, U32 len);

int sflash_test(void);
#endif

