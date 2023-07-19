#include "nvm.h"
#include "dal_nor.h"
#include "fs.h"
#include "dal.h"
#include "cfg.h"


int nvm_init(void)
{
    int r;
    
#ifdef USE_FS
    //r = fs_init(DEV_MMC, FS_FATFS);
#else
    #ifdef USE_EEPROM
        r = at24cxx_init();
    #else
        r = dal_nor_init();
    #endif
#endif
    
    return r;
}


int nvm_read(int id, void *data, int len)
{
    int r;
    U32 addr=0;
    
#ifdef USE_FS
    //r = fs_read(id, data, len);
#else
    #ifdef USE_EEPROM
        r = at24cxx_read(addr, (U8*)data, len);
    #else
        r = dal_nor_read(addr, (U8*)data, len);
    #endif
#endif
    
    return r;
}


int nvm_write(int id, void *data, int len)
{
    int r=0;
    U32 addr=0;
    
#ifdef USE_FS
    //r = fs_write(id, data, len, 0);
#else
    #ifdef USE_EEPROM
        r = at24cxx_write(addr, (U8*)data, len);
    #else
        r = dal_nor_write(addr, (U8*)data, len);
    #endif
#endif
    
    return r;
}



int nvm_remove(int id)
{
    int r=0;
    
#ifdef USE_FS
    //r = fs_remove(id);
#endif
    
    return r;
}


int nvm_sync(int id)
{
    int r=0;
    
#ifdef USE_FS
    //r = fs_sync(id);  
#endif
    
    return r;
}


