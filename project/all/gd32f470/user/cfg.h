#ifndef __CFG_Hx__
#define __CFG_Hx__


#define FW_VERSION                  "V1.0.0"


#define BOARD_V134
//#define BOARD_V136


#ifdef BOARD_V134
    #define RS485_PORT              UART_2
    #define I2C_SOFT                //ʹ��gpioģ��I2C
#elif defined BOARD_V136
    #define RS485_PORT              UART_5
    #define I2C_HARD_PIN            //ʹ��Ӳ��I2C
#else
    #error no board define!
#endif


#define BOOT_ADDR                   0x8000000

#define LOG_UART                    UART_0
#define LOG_BAUDRATE                (115200)

#define COM_UART                    UART_0
#define COM_BAUDRATE                (115200)


#define WDG_TIME                    4000        //���Ź�ι��ʱ������Ϊ4 seconds


#if 1                               
    
    #define USE_FS
    #ifdef USE_FS
    //#define USE_YAFFS
    #define USE_FATFS
    //#define USE_UBIFS
        
    #define SDMMC_MNT_PT             "/sd"
    #define SDMMC_FS_TYPE            FS_FATFS
    
    #define SFLASH_MNT_PT            "/sf"
    #define SFLASH_FS_TYPE           FS_FATFS
    
    
    #define SAV_DATA_PATH            SDMMC_MNT_PT"/data"
    
    #endif
    
    
    #define SYS_FREQ                (240*MHZ)
    #define USE_NET_MODULE
   
   
   #define SAMPLE_INT_INTERVAL      10  //�����жϼ��ʱ��, ��λ: ms
   
    #define SPI_PWM_CS
    //#define SPI_QUAD                //need exchange the miso and mosi
    //#define GPIO_SPI

    //#define ADC_CONV_FLOAT
    
    #define SPI_MODE                 0  
    
    
    
    
    
    //#define PROD_V2                 //�ַ�����ʽ��֧�������ϴ�
    #define PROD_V3                 //�Զ����ʽ��֧�ֶ������ϴ�
    
    #define USE_LAB_1
    //#define USE_LAB_2
    //#define USE_LAB_3
    //#define USE_LAB_4
    
    
    //#define DEV_MODE_NORM
    //#define DEV_MODE_TEST
    #define DEV_MODE_DEBUG
    
    
    //#define USE_PT1000
    #define AUTO_CAP
        
    #ifdef USE_LAB_2
        #define COUNTDOWN_POWER     //2���豸PA1�Ż��ˣ�����RTC�����ص�
    #endif
    
    //�ػ�����
    #define COUNTDOWN_MIN           5
    #define RUNTIME_MAX             30
    
    #if (defined DEV_MODE_NORM || defined DEV_MODE_TEST)
        #define USE_WDG                 //ʹ�ܿ��Ź�
    #endif
    
#endif


#ifdef  USE_EEPROM
    #define UPG_INFO_OFFSET         0
    #define APP_OFFSET              (0x8000)            //32KB--->The End
#else
    #define UPG_INFO_OFFSET         (0xC000)            //48KB
    
    
    #define FAC_APP_OFFSET          0
    #define TMP_APP_OFFSET          (1*MB)
    
    #define APP_OFFSET              (0x10000)           //64KB--->The End
#endif


#endif

