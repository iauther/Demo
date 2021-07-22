#ifndef __CFG_Hx__
#define __CFG_Hx__


//#define BOARD_JKCARE_V03




#define VERSION                     "V1.0.0.0.000"

#define BOOT_OFFSET                 (0)                 //32KB space reserved
#define COM_BAUDRATE                (115200)


#ifdef BOARD_JKCARE_V03
    #define I2C0_FREQ               HAL_I2C_FREQUENCY_100K
    #define I2C0_SCL_PIN            {0, HAL_GPIO_36}
    #define I2C0_SDA_PIN            {0, HAL_GPIO_37}
    
    #define I2C2_FREQ               HAL_I2C_FREQUENCY_100K
    #define I2C2_SCL_PIN            {0, HAL_GPIO_8}
    #define I2C2_SDA_PIN            {0, HAL_GPIO_9}
    
    
    #define USE_TCA6424A
    
#else
    #define I2C1_FREQ               (100*1000)
    //#define I2C1_SCL_PIN            {GPIOA, GPIO_PIN_8}
    //#define I2C1_SDA_PIN            {GPIOC, GPIO_PIN_9}
    
    #define I2C2_FREQ               (100*1000)
    //#define I2C2_SCL_PIN            {GPIOF, GPIO_PIN_1}
    //#define I2C2_SDA_PIN            {GPIOF, GPIO_PIN_0}
    
    #define GPIO_PAD_PWR_PIN        {HAL_GPIO_MAX}
    
    #define USE_TCA6424A
    
    
#endif


#ifdef  USE_EEPROM
    #define UPGRADE_INFO_ADDR       0
    #define APP_OFFSET              (0x8000)            //32KB--->The End
#else
    #define UPGRADE_INFO_ADDR       (0x8000)
    #define APP_OFFSET              (0x10000)           //64KB--->The End
#endif


#endif

