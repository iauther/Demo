#ifndef __BMI160_H__
#define __BMI160_H__

#include "bmi160/bmi160_ori.h"
#include "bmi160/bmi160_defs_ori.h"



//定义bmi160 I2C端口号
#define	BMI160_I2C_NUM					HAL_I2C_MASTER_0
 
//BMI160 I2C 设备地址
#define BMI160_ADDRESS                  0x68  

//定义动作唤醒陀螺仪参数
#define ANYMOTION_SET_DUR               0           //范围0-3    动作持续(设置值+1)个点唤醒陀螺仪 
#define ANYMOTION_SET_THR               10          //范围0-255  判断动作阈值
                                                    //量程为2g-设置值*3.91mg  4g-7.81mg  8g-15.63mg  16g-31.25mg
                                                    
                                                    
//定义静止休眠陀螺仪参数
#define NOMOTION_SET_DUR                0           //范围0-63    静止持续多长时间(设置值+1)休眠陀螺仪 
                                                   //DUR <= 15时，        设置时间=(DUR+1)*1.28秒      
                                                   //DUR>=16 且 DUR<32时，设置时间=(DUR-16 + 5)*5.12秒  
                                                   //DUR>=32 且 DUR<64时，设置时间=(DUR-32 + 11)*10.24秒                                                         
#define NOMOTION_SET_THR                8          //范围0-255  判断静止阈值
                                                   //量程为2g-设置值*3.91mg  4g-7.81mg  8g-15.63mg  16g-31.25mg  


#define PMU_TRIGGER_SET                 0x34;      //0b00110100配置为anymotion唤醒gyro，nomotion触发gyro睡眠

//驱动定义返回0为成功，其余数为失败
#define BMI160_ERROR                    -1

//定义BMI160 I2C 超时时间(毫秒）
#define BMI160_I2C_TIMEOUT              10

//定义BMP280通讯最多错误次数
#define BMI160_ERROR_TIMES              100

//定义官方驱动未定义的寄存器
#define BMI160_PMU_STATUS_ADDR		     UINT8_C(0x03)
#define BMI160_PMU_TRIGGER_ADDR		     UINT8_C(0x6C)

//定义官方驱动未定义的命令
#define BMI160_STEP_CLR_CMD		     	0xB2

//外部接口函数
int8_t bmi160_init(void);
int8_t bmi160_deinit(void);
int8_t bmi160_reset(void);

int8_t bmi160_read_accelgyro(uint8_t reg_addr, uint8_t *pData);
int8_t bmi160_read_accelgyro_dma(uint8_t reg_addr, uint8_t *pData);
int8_t bmi160_get_error_code(uint8_t *error_code);
int8_t bmi160_get_pmu_status(uint8_t *pmu_status);
int8_t bmi160_get_status(void);
int8_t bmi160_get_step(uint16_t *step_val);
int8_t bmi160_clr_step(void);
void bmi160_check_reg(void);

#endif

