#include "nvm.h"
#include "fs.h"
#include "dal.h"
#include "cfg.h"


int nvm_init(void)
{
    int r;
    
#ifdef USE_FS
    fs_init();
#else
    #ifdef USE_EEPROM
        r = at24cxx_init();
    #else
        r = flash_init();
    #endif
#endif
    
    return r;
}


int nvm_read(int id, void *data, int len)
{
    int r;
    
#ifdef USE_FS
    r = fs_read(id, data, len);
#else
    #ifdef USE_EEPROM
        r = at24cxx_read(addr, (U8*)data, len);
    #else
        r = flash_read(addr, (U8*)data, len);
    #endif
#endif
    
    return r;
}


int nvm_write(int id, void *data, int len)
{
    int r=0;
    
#ifdef USE_FS
    r = fs_write(id, data, len);
#else
    #ifdef USE_EEPROM
        r = at24cxx_write(addr, (U8*)data, len);
    #else
        r = flash_write(addr, (U8*)data, len);
    #endif
#endif
    
    return r;
}



int nvm_remove(int id)
{
    int r=0;
    
#ifdef USE_FS
    fs_remove(id);
#else
    
#endif
    
    return r;
}


int nvm_sync(int id)
{
    int r=0;
    
#ifdef USE_FS
    fs_sync(id);
#else
    
#endif
    
    return r;
}


