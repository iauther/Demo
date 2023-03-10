#ifndef __CFG_Hx__
#define __CFG_Hx__


#define VERSION                     "V1.0.0"

#define BOOT_OFFSET                 (0)                 //32KB space reserved
#define COM_UART_PORT               UART_3
#define COM_BAUDRATE                (115200)


#define MHZ                         (1000*1000)

#if 1
    
    #define I2C1_FREQ               (100*1000)
    #define I2C1_SCL_PIN            {GPIOA, GPIO_PIN_8}
    #define I2C1_SDA_PIN            {GPIOC, GPIO_PIN_9}
    
    
    //#define USE_EEPROM
    #define USE_AT24C16
    #define AT24CXX_A0_PIN          0
    #define AT24CXX_A1_PIN          0
    #define AT24CXX_A2_PIN          0  
    
    
    #define SYS_FREQ                (480*MHZ)
    //#define SYS_FREQ                (384*MHZ)
    
    
    
    
    #define ADC_DURAL_MODE          1

    //#define ADC_SIGNAL_MODE      ADC_SINGLE_ENDED
    #define ADC_SIGNAL_MODE      ADC_DIFFERENTIAL_ENDED
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

#if 1
    #define ADC_SAMPLE_CHANS    {\
                                    {{GPIOA, GPIO_PIN_4}, ADC_CHANNEL_18, ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_1, ADC_DIFFERENTIAL_ENDED, POLAR_P}, \
                                    {{GPIOA, GPIO_PIN_5}, ADC_CHANNEL_18, ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_1, ADC_DIFFERENTIAL_ENDED, POLAR_N}, \
                                    {{GPIOB, GPIO_PIN_0}, ADC_CHANNEL_5,  ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_1, ADC_DIFFERENTIAL_ENDED, POLAR_N}, \
                                    {{GPIOB, GPIO_PIN_1}, ADC_CHANNEL_5,  ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_1, ADC_DIFFERENTIAL_ENDED, POLAR_P}, \
                                    {{0, 0},              0,              0,                   0,         0,                  0,                      0} \
                                }
#else
    #define ADC_SAMPLE_CHANS    {\
                                    {{GPIOA, GPIO_PIN_4}, ADC_CHANNEL_18, ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_1, ADC_DIFFERENTIAL_ENDED, POLAR_P}, \
                                    {{GPIOA, GPIO_PIN_5}, ADC_CHANNEL_18, ADC_RESOLUTION_16B, (500*1000), ADC_REGULAR_RANK_1, ADC_DIFFERENTIAL_ENDED, POLAR_N}, \
                                    {{0, 0},              0,              0,                   0,         0,                  0,                      0} \
                                }


#endif
 
    
#endif


#ifdef  USE_EEPROM
    #define UPGRADE_INFO_ADDR       0
    #define APP_OFFSET              (0x8000)            //32KB--->The End
#else
    #define UPGRADE_INFO_ADDR       (0x8000)
    #define APP_OFFSET              (0x10000)           //64KB--->The End
#endif


#endif

