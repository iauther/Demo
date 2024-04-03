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
    ERROR_PKT_ID,
    ERROR_PKT_TYPE_UNSUPPORT,
    ERROR_PKT_PARA,
    
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
    ERROR_NET_DISCONN,
    
    //system
    ERROR_I2C1_GEN=0x60,      //all i2c dev except for eeprom
    ERROR_I2C2,
    ERROR_COMM_ETH,
    ERROR_COMM_USB,
    ERROR_COMM_UART,
    

    
    ERROR_MAX
    
};



#endif

