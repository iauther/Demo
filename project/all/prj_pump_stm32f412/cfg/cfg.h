#ifndef __CFG_Hx__
#define __CFG_Hx__



#define VERSION                 "V1.0.0"




#define BOOT_OFFSET             (0)                 //32KB
#define APP_OFFSET              (0x8000)            //32KB--->The End

#define USE_AT24C16
#define AT24CXX_A0_PIN 0
#define AT24CXX_A1_PIN 0
#define AT24CXX_A2_PIN 0



//#define UPGRADE_STORE_IN_FLASH
#define UPGRADE_STORE_IN_E2PROM


#define UPGRADE_STORE_ADDR      0















#endif

