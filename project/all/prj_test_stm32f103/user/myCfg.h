#ifndef __CFG_Hx__
#define __CFG_Hx__


#define VERSION                         "V1.0"


#ifdef TEST_BOARD_3IN1

    #define LCD_ST7789
    #define LCD_WIDTH                   (240)
    #define LCD_HEIGHT                  (240)
    #define LCD_COLOR                   (COLOR_RGB565)


    #define LCD_RST_PIN                 {GPIOC, GPIO_PIN_0}
    #define LCD_CMD_PIN                 {GPIOC, GPIO_PIN_1}
    #define LCD_BLK_PIN                 {GPIOC, GPIO_PIN_13}
    #define LCD_CLK_PIN                 {GPIOA, GPIO_PIN_5}
    #define LCD_DAT_PIN                 {GPIOA, GPIO_PIN_7}
    

    #define I2C1_FREQ                   (100*1000)
    #define I2C1_SCL_PIN                {GPIOB, GPIO_PIN_6}
    #define I2C1_SDA_PIN                {GPIOB, GPIO_PIN_7}

    #define VOLT_S0_PIN                 {GPIOB, GPIO_PIN_0}
    #define VOLT_S1_PIN                 {GPIOB, GPIO_PIN_1}
    #define VOLT_S2_PIN                 {GPIOB, GPIO_PIN_2}
    #define VOLT_S3_PIN                 {GPIOA, GPIO_PIN_2}//{GPIOB, GPIO_PIN_3}

    #define VOLT_ADC1                   {GPIOA, GPIO_PIN_0}
    #define VOLT_ADC2                   {GPIOA, GPIO_PIN_1}

    #define TCP232_RX_PIN               {GPIOA, GPIO_PIN_10}
    #define TCP232_TX_PIN               {GPIOA, GPIO_PIN_9}
    #define TCP232_RST_PIN              {GPIOD, GPIO_PIN_2}
    #define TCP232_CFG_PIN              {GPIOC, GPIO_PIN_15}

    #define KEY1_PIN                    {GPIOC, GPIO_PIN_3}
    #define KEY2_PIN                    {GPIOC, GPIO_PIN_4}
    #define KEY3_PIN                    {GPIOC, GPIO_PIN_5}
    #define KEY4_PIN                    {GPIOC, GPIO_PIN_6}
    #define KEY5_PIN                    {GPIOC, GPIO_PIN_7}
    #define KEY6_PIN                    {GPIOC, GPIO_PIN_8}
    
    #define POWER_EN                    {GPIOB, GPIO_PIN_8}
    
    

    #define AT24CXX_ADDR                0x23
    #define GPIO_AT24CXX_PIN            {GPIOC, GPIO_PIN_14}

#elif defined TEST_BOARD_LEAD_SWITCH
    
    
#else
    error "must define a board first!!"
#endif
    
    

#endif

