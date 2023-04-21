#ifndef __CFG_Hx__
#define __CFG_Hx__


#define VERSION                     "V1.0.0"

#define BOOT_OFFSET                 (0)                 //32KB space reserved


#define LOG_UART                    UART_3
#define LOG_BAUDRATE                (115200)

#define COM_UART                    UART_4
#define COM_BAUDRATE                (115200)

#define ADC_MUX_ENABLE



#define SDRAM_ADDR                  0xD0000000
//#define SDRAM_SIZE                  (64*1024*1024)

#define XMEM_ADDR                   (SDRAM_ADDR+0)
#define XMEM_LEN                    (50*1024*1024)



#define MHZ                         (1000*1000)
#define KB                          (1024)
#define MB                          (1024*1024)

#if 1
   
/*
    一共2路I2C总线， 分别为：
    I2C_1： SCL:PH4   SDA:PH5， 用于底板，   2片pcm1864(地址分别为：0x4c(CH1~CH4), 0x4d(CH5~CH8)) 或 2片adau1978(地址分别为0x11(CH1~CH4), 0x31(CH5~CH8), 目前没贴)，两类芯片2选1
    I2C_2： SCL:PB6   SDA:PB7， 用于扩展板， 2片pcm1864(地址分别为：0x4c, 0x4d) 或 2片adau1978(地址分别为0x11, 0x31, 目前没贴)
*/

    #define I2C1_FREQ               (100*1000)
    #define I2C1_SCL_PIN            {GPIOH, GPIO_PIN_4}
    #define I2C1_SDA_PIN            {GPIOH, GPIO_PIN_5}
                                    
    #define I2C2_FREQ               (100*1000)
    #define I2C2_SCL_PIN            {GPIOB, GPIO_PIN_6}
    #define I2C2_SDA_PIN            {GPIOB, GPIO_PIN_7}
                                    
    
    #define USE_FS
    #ifdef USE_FS
    #define USE_YAFFS
    //#define USE_FATFS
    //#define USE_UBIFS
    #endif
    
    #define USE_NOR
    //#define USE_NAND
    //#define USE_EEPROM
    
    #ifdef USE_EEPROM
        #define USE_AT24C16
        #define AT24CXX_A0_PIN          0
        #define AT24CXX_A1_PIN          0
        #define AT24CXX_A2_PIN          0  
    #elif defined USE_NOR
        #define USE_SPI_NOR
        //#define USE_BLT_NOR
    #elif defined USE_NAND
        #define USE_SPI_NAND
        //#define USE_FMC_NAND
    #endif
    
    
    #define SYS_FREQ                (480*MHZ)
    //#define SYS_FREQ                (384*MHZ)
    
    
    
    #define CPU_IPL_BOUNDARY       4
    #define INT_PRI_SAI            (CPU_IPL_BOUNDARY - 1)   //← SAI溢出中断抢占优先级4-1
    #define INT_PRI_SPD            (0 + CPU_IPL_BOUNDARY)   //← 转速中断抢占优先级0+4
    #define INT_PRI_VIB            (1 + CPU_IPL_BOUNDARY)   //← 振动中断抢占优先级1+4
    #define INT_PRI_ISO            (1 + CPU_IPL_BOUNDARY)   //← 过程量中断抢占优先级1+4
    #define INT_PRI_ETH            (2 + CPU_IPL_BOUNDARY)   //← ETH中断抢占优先级2+4
    #define INT_PRI_RS485          (2 + CPU_IPL_BOUNDARY)   //← RS485中断抢占优先级2+4
    
    
    #define MOUNT_POINT             "/"
    
    
    
    #define ADC_DUAL_MODE          1
    #define ADC_SAMPLE_COUNT       4000


    #define USE_ADC_EXT_BOARD

    //#define ADC_SIGNAL_MODE      ADC_SINGLE_ENDED
    #define ADC_SIGNAL_MODE      ADC_DIFFERENTIAL_ENDED
    
    #define COM_RX_BUF_LEN         (ADC_SAMPLE_COUNT*4*2+100)
   




    //NAND SPACE DEFINE  1 page == 2KB
    //meta 8kB
    #define NAND_META_START             0
    #define NAND_META_LEN               (8*KB)
    
    //boot 128kB
    #define NAND_BOOT_START             (NAND_META_START+NAND_META_LEN)
    #define NAND_BOOT_LEN               (120*KB)
    
    //meta 1024kB
    #define NAND_APP_START              (NAND_BOOT_START+NAND_BOOT_LEN)
    #define NAND_APP_LEN                (1024*KB)

    #define NAND_FS_START               (NAND_APP_START+NAND_APP_LEN)
    #define NAND_FS_LEN                 (500*MB)
    
#endif


#ifdef  USE_EEPROM
    #define UPGRADE_INFO_ADDR       0
    #define APP_OFFSET              (0x8000)            //32KB--->The End
#else
    #define UPGRADE_INFO_ADDR       (0x8000)
    #define APP_OFFSET              (0x10000)           //64KB--->The End
#endif


#endif

