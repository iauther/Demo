#ifndef __ERROR_Hx__
#define __ERROR_Hx__


enum {
    ERROR_NONE=0,
    
    //device
    ERROR_DEV_PUMP_RW=0x10,
    ERROR_DEV_MS4525_RW,
    ERROR_DEV_E2PROM_RW,
    
    //packet hdr
    ERROR_PKT_TYPE=0x20,
    ERROR_PKT_MAGIC,
    ERROR_PKT_LENGTH,
    ERROR_PKT_CHECKSUM,
    
    //data
    ERROR_DAT_MODE=0x30,
    ERROR_DAT_VACUUM,
    ERROR_DAT_VOLUME,
    ERROR_DAT_TIMESET,
    
    ERROR_CMD_CODE,
    ERROR_CMD_PARA,
    
    //firmware
    ERROR_FW_INFO_VERSION=0x40,
    ERROR_FW_INFO_BLDTIME,
    ERROR_FW_PKT_ID,
    ERROR_FW_PKT_LENGTH,
    
    ERROR_FW_OLD_VERSION,
    ERROR_FW_OLD_BLDTIME,
    ERROR_FW_MAGIC_WRONG,
    ERROR_FW_FLASH_WRITE,
    ERROR_FW_EEPROM_WRITE,
    
    //communication
    ERROR_ACK_TIMEOUT=0x50,
    
    //system
    ERROR_I2C1_GEN=0x60,      //all i2c dev except for eeprom
    ERROR_I2C2_E2P,          //eeprom
    ERROR_PWM_PUMP,
    ERROR_UART1_PUMP,        //pump
    ERROR_UART2_COM,         //ipad uart port
    
    ERROR_MAX
    
};



#endif

