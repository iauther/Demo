#ifndef __CFG_Hx__
#define __CFG_Hx__

//      PROJECT DEFINE
#define PRJ_MULTI_CHANNEL_ECGRECODER


#define APP_OFFSET              (0x8000)
#define BOOT_OFFSET             (0)

#define E2P_SIZE                (1024)
#define UPGRADE_INFO_ADDR       (E2P_SIZE-100)
#define UPGRADE_FLAG_ADDR       (0x2000000+100*1024)


#define USART_COMM              (USART_2)








//////////////////PROJECT DETAIL DEFINE///////////////////////
#ifdef PRJ_MULTI_CHANNEL_ECGRECODER
    #define CHIP_MT2523
    //#define USE_FREERTOS



    //#define BOARD_V00_01
    #define BOARD_V00_02

    #define KHZ                                 (1000)
    #define MHZ                                 (1000000)
    
    #define SPI_CS_PIN                          (HAL_GPIO_11)
    #define SPI_CLK_PIN                         (HAL_GPIO_12)
    #define SPI_MOSI_PIN                        (HAL_GPIO_13)
    #define SPI_MISO_PIN                        (HAL_GPIO_14)
    
    
    #define I2C0_FREQ                           (HAL_I2C_FREQUENCY_200K)
    #define I2C0_SCL_PIN                        (HAL_GPIO_36)
    #define I2C0_SDA_PIN                        (HAL_GPIO_37)
    
    #define BIO_POWER_PIN		                (HAL_GPIO_10)
    
    #ifdef BOARD_V00_01
    
        #define AD8233_FREQ                     (8*MHZ)
        
        #define ADS129X_ENABLE
        #define ADS129X_FREQ                    (4*MHZ)
        #define ADS129X_DRDY_PIN                (HAL_GPIO_24)
        #define ADS129X_RESET_PIN               (HAL_GPIO_5)    //UTXD1
        #define ADS129X_START_PIN               (HAL_GPIO_4)    //URXD1
        
        #define BIO_DETECT_PIN                  (HAL_GPIO_MAX)
        
    #elif defined BOARD_V00_02
        
        #define AD8233_FREQ                     (8*MHZ)
        
        #define TEA6424A_I2C_FREQ               (HAL_I2C_FREQUENCY_200K)
        #define TEA6424A_I2C_SDA_PIN            (HAL_GPIO_43)
        #define TEA6424A_I2C_SCL_PIN            (HAL_GPIO_38)
        
        //#define ADS129X_ENABLE
        #define ADS129X_FREQ                    (4*MHZ)
        #define ADS129X_DRDY_PIN                (HAL_GPIO_24)   //
        #define ADS129X_RESET_PIN               (HAL_GPIO_39)   //LSCE_B
        #define ADS129X_START_PIN               (HAL_GPIO_40)   //LSCK
        
        #define I2C1_SCL_PIN                    (HAL_GPIO_38)   //LSRSTB
        #define I2C1_SDA_PIN                    (HAL_GPIO_43)   //LPTE
        
        #define BIO_DETECT_PIN                  (HAL_GPIO_15)  //gpio c4
    #endif


#endif  //PRJ_MULTI_CHANNEL_ECGRECODER










#endif  //__CFG_Hx__







