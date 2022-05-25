#ifndef __CFG_Hx__
#define __CFG_Hx__


#define VERSION                         "V1.0"



#ifdef TEST_MISTRIG_BOX

    #define LCD_YAOXY
    #ifdef LCD_YAOXY
        #define BUILTIN_FONT
    #endif
    
    
    //#define LCD_WIDTH                   (16)
    //#define LCD_HEIGHT                  (8)
    #define LCD_COLOR                   (COLOR_RGB565)
    
    //LCD SPI
    #define LCD_SPI_FREQ                (500*1000)
    #define LCD_SPI_PORT                 SPI_1   
    
    
    #define PSEL_PIN                    {GPIOA, GPIO_PIN_0}
    #define FSEL_PIN                    {GPIOA, GPIO_PIN_0}
    
    #define I2C1_FREQ                   (100*1000)
    #define I2C1_SCL_PIN                {GPIOB, GPIO_PIN_6}
    #define I2C1_SDA_PIN                {GPIOB, GPIO_PIN_7}
    
    //AD9834 SPI
    #define AD9834_SPI_FREQ              (500*1000)
    #define AD9834_SPI_PORT              SPI_1
    #define AD9834_SPI_CS_PIN           {GPIOB, GPIO_PIN_12}
    #define AD9834_RESET_PIN            {GPIOD, GPIO_PIN_13}
    
    
    #define KEY_CLR_PIN                 {GPIOE, GPIO_PIN_2}     //KEY1
    #define KEY_REC_PIN                 {GPIOE, GPIO_PIN_0}     //KEY2
    
    
    
    
    //module 4 lines interface
    #define MOD_O_PIN                   {GPIOA, GPIO_PIN_3}     //ADC_O1
    #define MOD_R_PIN                   {GPIOA, GPIO_PIN_2}     //RX1
    #define MOD_Z_PIN                   {GPIOC, GPIO_PIN_5}     //Z1
    
    
    #define VCTL_DAC_PIN                {GPIOA, GPIO_PIN_4}     //DA_V
    //#define VOUT_DET_PIN                {GPIOC, GPIO_PIN_5}
    
    
    //magnet trigger pin
    #define MOTION_TRIG_PIN             {GPIOA, GPIO_PIN_0}     //high on, low off
    
    #define BUZZER_PIN                  {GPIOD, GPIO_PIN_14}    //BUZ_PWM
    #define SYS_LED_PIN                 {GPIOE, GPIO_PIN_1}     //LED, system status led, 1: on, 0: off
    #define MOD_LED_PIN                 {GPIOXX, GPIO_PIN_XXX}     //????
    
    
    #define ADC1_OUT_PIN                {GPIOC, GPIO_PIN_0}     //ADC_UOUT, master voltage capture
    #define ADC2_OUT_PIN                {GPIOA, GPIO_PIN_6}     //ADC_VT,   module io voltage capture
    #define ADC3_OUT_PIN                {GPIOC, GPIO_PIN_4}     //ADC_V,    module input voltage capture
    #define ADC4_OUT_PIN                {GPIOA, GPIO_PIN_1}     //ADC_IOUT, module current capture
    
    
    #define USE_EEPROM
    #define USE_AT24C02
    #define AT24CXX_A0_PIN              0
    #define AT24CXX_A1_PIN              0
    #define AT24CXX_A2_PIN              0
    #define GPIO_AT24CXX_WP_PIN         {GPIOD, GPIO_PIN_9}
    
    #define ESP8266_PORT                UART_3
    #define ESP8266_BAUDRATE            115200
    
    
#else
    error "must define a board first!!"
#endif

    
#ifdef  USE_EEPROM
    #define UPGRADE_INFO_ADDR       0
    #define APP_OFFSET              (0x8000)            //32KB--->The End
#else
    #define UPGRADE_INFO_ADDR       (0x8000)
    #define APP_OFFSET              (0x10000)           //64KB--->The End
#endif
    
    
    

#endif

