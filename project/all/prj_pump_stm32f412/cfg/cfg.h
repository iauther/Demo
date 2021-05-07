#ifndef __CFG_Hx__
#define __CFG_Hx__



#define VERSION                     "V1.0.0"

#define BOOT_OFFSET                 (0)                 //32KB
#define COM_BAUDRATE                (115200)


#define USE_EEPROM
#ifdef  USE_EEPROM
    #define USE_AT24C16
    #define UPGRADE_INFO_ADDR       0
    #define APP_OFFSET              (0x8000)            //32KB--->The End
#else
    #define UPGRADE_INFO_ADDR       (0x8000)
    #define APP_OFFSET              (0x10000)           //64KB--->The End
#endif


#endif

