#ifndef __ERROR_Hx__
#define __ERROR_Hx__


enum {
    ERROR_NONE=0,
    
    //device
    ERROR_DEV_PUMP_RW=0x10,
    ERROR_DEV_MS4525_RW,
    ERROR_DEV_E2PROM_RW,
    
    //packet
    ERROR_PKT_TYPE=0x20,
    ERROR_PKT_MAGIC,
    ERROR_PKT_LENGTH,
    ERROR_PKT_CHECKSUM,
    
    //data
    ERROR_DAT_MODE=0x30,
    ERROR_DAT_VACUUM,
    ERROR_DAT_VOLUME,
    ERROR_DAT_TIMESET,
    
    //firmware
    ERROR_FW_INFO_VERSION=0x40,
    ERROR_FW_INFO_BLDTIME,
    ERROR_FW_PKT_ID,
    ERROR_FW_PKT_LENGTH,
    
    //communication
    ERROR_ACK_TIMEOUT=0x50,
    ERROR_UPG_FAILED,
    
    //system
    ERROR_SYS_I2C1=0x60,
    ERROR_SYS_I2C2,
    ERROR_SYS_UART1,
    ERROR_SYS_UART2,
    
     
};



#endif

