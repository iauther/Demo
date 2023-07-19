#ifndef __CFG_Hx__
#define __CFG_Hx__


#define VERSION                     "V1.0.0"

#define BOOT_ADDR                   0x8000000

#define LOG_UART                    UART_0
#define LOG_BAUDRATE                (115200)

#define COM_UART                    UART_4
#define COM_BAUDRATE                (115200)

#define KHZ                         (1000)
#define MHZ                         (1000*KHZ)
#define KB                          (1024)
#define MB                          (1024*1024)

#if 1                               
    
    #define USE_FS
    #ifdef USE_FS
    //#define USE_YAFFS
    #define USE_FATFS
    //#define USE_UBIFS
    
    #define MOUNT_POINT              "/"
    
    #define CFG_DIR                  "/cfg"
    #define DATA_DIR                 "/data"
    
    #endif
 
    
    #define SYS_FREQ                (240*MHZ)
    #define USE_NET_MODULE
   
   
   #define ADC_BORD_V11             //new adc board
   
    #define SPI_PWM_CS
    //#define SPI_QUAD                //need exchange the miso and mosi
    //#define GPIO_SPI

    //#define ADC_CONV_FLOAT

    #define DAC_OUTPUT
    #define DAC_FREQ_DIV             1//10
    
    #define SPI_MODE                 0  

    #define SAMPLE_INV_TIME          10     //seconds


    #define SAMPLE_FREQ             (500*KHZ)
    #define ONCE_SAMPLE_TIME        20              //ms      


    #define SAMPLE_POINTS           200
    #define COMM_BUF_LEN            (SAMPLE_POINTS*4+32)
    
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

