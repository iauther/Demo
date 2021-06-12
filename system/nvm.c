#include "at24cxx.h"
#ifndef _WIN32
#include "drv/flash.h"
#endif
#include "nvm.h"
#include "myCfg.h"



#ifdef _WIN32
static FILE* paras_fp;
#endif

int nvm_init(void)
{
    int r;
    #ifdef _WIN32
    paras_fp = fopen("paras.dat", "wb");
    r = paras_fp?0:-1;
#else
    #ifdef USE_EEPROM
        r = at24cxx_init();
    #else
        r = flash_init();
    #endif
#endif
    
    return r;
}


int nvm_read(U32 addr, U8 *data, U32 len)
{
    int r;
    
#ifdef _WIN32
    fseek(paras_fp, addr, SEEK_SET);
    r = fread(data, 1, len, paras_fp);
#else
    #ifdef USE_EEPROM
        r = at24cxx_read(addr, (U8*)data, len);
    #else
        r = flash_read(addr, (U8*)data, len);
    #endif
#endif
    
    return r;
}


int nvm_write(U32 addr, U8 *data, U32 len)
{
    int r;
    
#ifdef _WIN32
    fseek(paras_fp, addr, SEEK_SET);
    r = fwrite(data, 1, len, paras_fp);
#else
    #ifdef USE_EEPROM
        r = at24cxx_write(addr, (U8*)data, len);
    #else
        r = flash_write(addr, (U8*)data, len);
    #endif
#endif
    
    return r;
}



int nvm_erase(U32 addr, U8 *data, U32 len)
{
    return 0;
}


