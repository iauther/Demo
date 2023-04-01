#ifndef __CFG_Hx__
#define __CFG_Hx__


#define VERSION                     "V1.0.0"

#define BOOT_OFFSET                 (0)                 //32KB space reserved


#define LOG_UART                    UART_3
#define LOG_BAUDRATE                (115200)

#define COM_UART                    UART_3
#define COM_BAUDRATE                (115200)

#define ADC_MUX_ENABLE



#define SDRAM_ADDR                  0xD0000000
//#define SDRAM_SIZE                  (64*1024*1024)
#define RTX_MEM_SIZE                 (50*1024*1024)



#define MHZ                         (1000*1000)

#if 1
   
/*
    һ��2·I2C���ߣ� �ֱ�Ϊ��
    I2C_1�� SCL:PH4   SDA:PH5�� ���ڵװ壬   2Ƭpcm1864(��ַ�ֱ�Ϊ��0x4c(CH1~CH4), 0x4d(CH5~CH8)) �� 2Ƭadau1978(��ַ�ֱ�Ϊ0x11(CH1~CH4), 0x31(CH5~CH8), Ŀǰû��)������оƬ2ѡ1
    I2C_2�� SCL:PB6   SDA:PB7�� ������չ�壬 2Ƭpcm1864(��ַ�ֱ�Ϊ��0x4c, 0x4d) �� 2Ƭadau1978(��ַ�ֱ�Ϊ0x11, 0x31, Ŀǰû��)
*/

    #define I2C1_FREQ               (100*1000)
    #define I2C1_SCL_PIN            {GPIOH, GPIO_PIN_4}
    #define I2C1_SDA_PIN            {GPIOH, GPIO_PIN_5}
                                    
    #define I2C2_FREQ               (100*1000)
    #define I2C2_SCL_PIN            {GPIOB, GPIO_PIN_6}
    #define I2C2_SDA_PIN            {GPIOB, GPIO_PIN_7}
                                    
    
    
    //#define USE_EEPROM
    #define USE_AT24C16
    #define AT24CXX_A0_PIN          0
    #define AT24CXX_A1_PIN          0
    #define AT24CXX_A2_PIN          0  
    
    
    #define SYS_FREQ                (480*MHZ)
    //#define SYS_FREQ                (384*MHZ)
    
    
    
    #define CPU_IPL_BOUNDARY       4
    #define INT_PRI_SAI            (CPU_IPL_BOUNDARY - 1)   //�� SAI����ж���ռ���ȼ�4-1
    #define INT_PRI_SPD            (0 + CPU_IPL_BOUNDARY)   //�� ת���ж���ռ���ȼ�0+4
    #define INT_PRI_VIB            (1 + CPU_IPL_BOUNDARY)   //�� ���ж���ռ���ȼ�1+4
    #define INT_PRI_ISO            (1 + CPU_IPL_BOUNDARY)   //�� �������ж���ռ���ȼ�1+4
    #define INT_PRI_ETH            (2 + CPU_IPL_BOUNDARY)   //�� ETH�ж���ռ���ȼ�2+4
    #define INT_PRI_RS485          (2 + CPU_IPL_BOUNDARY)   //�� RS485�ж���ռ���ȼ�2+4
    
    
    
    #define ADC_DUAL_MODE          1
    #define ADC_SAMPLE_COUNT       400


    #define USE_ADC_EXT_BOARD

    //#define ADC_SIGNAL_MODE      ADC_SINGLE_ENDED
    #define ADC_SIGNAL_MODE      ADC_DIFFERENTIAL_ENDED
    
    #define COM_RX_BUF_LEN         (ADC_SAMPLE_COUNT*4*2+100)
   

 
    
#endif


#ifdef  USE_EEPROM
    #define UPGRADE_INFO_ADDR       0
    #define APP_OFFSET              (0x8000)            //32KB--->The End
#else
    #define UPGRADE_INFO_ADDR       (0x8000)
    #define APP_OFFSET              (0x10000)           //64KB--->The End
#endif


#endif

