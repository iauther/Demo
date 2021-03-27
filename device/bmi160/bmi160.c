/**
  ==============================================================================
  * @file    bmi160.c
  * @author  向明君
  * @version V1.0
  * @date    2017-9-15
  * @brief   BMI160驱动程序接口及应用 bmi160_ori.c bmi160_ori.h bmi160_defs.h为博世官方下载未修改
             此文件主要是完善官方驱动提供通讯接口和应用
  @verbatim
  ==============================================================================
*/           
#include "bmi160/bmi160.h"
#include "hal.h"

//定义通讯出错统计
static uint16_t bmi160_error_times = 0;
//定义官方驱动操作bmi160全局控制结构体
struct bmi160_dev bmi160_sensor;

//私有函数
static void bmi160_delay_ms(uint32_t msec);
static void bmi160_i2c_error_deal(void);
static int8_t bmi160_i2c_bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t cnt);
static int8_t bmi160_i2c_bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t cnt);
static int8_t bmi160_app_int_set(void);
static int8_t bmi160_app_para_set(void);

//外部接口函数
//-------------------------------------------------------------------------------------------------
//底层通讯接口
/*-------------------------------------------------------------------------------------------------
  功能:  I2C接口初始化
  输入： 无
  返回： 无
--------------------------------------------------------------------------------------------------*/
static void bmi160_i2c_init(void)
{
	hal_i2c_config_t i2c_config;
	i2c_config.frequency = HAL_I2C_FREQUENCY_400K;
	hal_i2c_master_init(BMI160_I2C_NUM, &i2c_config);  
}  

/*-------------------------------------------------------------------------------------------------
  功能:  I2C接口反初始化
  输入： 无
  返回： 无
--------------------------------------------------------------------------------------------------*/
static void bmi160_i2c_deinit(void)
{
	hal_i2c_master_deinit(BMI160_I2C_NUM); 
}  


/*-------------------------------------------------------------------------------------------------
  功能:  收到中断后用I2C DMA方式读取BMI160的0x0c-0x17的12个数据输出寄存器值，包括角速度和加速度
  输入： pData接收数据指针，排列方式为角速度x,y,z ，加速度x,y,z 低字节在前，一共12个字节数据
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
int8_t bmi160_read_accelgyro_dma(uint8_t reg_addr, uint8_t *pData)
{ 	
	hal_i2c_send_to_receive_config_ex_t data_config;
	
	if(bmi160_get_status() != BMI160_OK) return BMI160_ERROR;
	
	data_config.slave_address  = BMI160_I2C_ADDR;
	data_config.send_data 	   = &reg_addr;	
	data_config.send_packet_length = 1;
	data_config.send_bytes_in_one_packet = 1;	
	data_config.receive_buffer = pData;
	data_config.receive_packet_length = 1;
	data_config.receive_bytes_in_one_packet = 12;	
	
	if(HAL_I2C_STATUS_OK == hal_i2c_master_send_to_receive_dma_ex(BMI160_I2C_NUM, &data_config)) 
	{
        bmi160_error_times = 0;			
        return BMI160_OK;
    }
    bmi160_i2c_error_deal();     
    return BMI160_ERROR;
}


/*-------------------------------------------------------------------------------------------------
  功能:  收到中断后用I2C方式读取BMI160的0x0c-0x17的12个数据输出寄存器值，包括角速度和加速度
  输入： pData接收数据指针，排列方式为角速度x,y,z ，加速度x,y,z 低字节在前，一共12个字节数据
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
int8_t bmi160_read_accelgyro(uint8_t reg_addr, uint8_t *pData)
{ 	
	hal_i2c_status_t i2c_status;
	hal_i2c_send_to_receive_config_t data_config;
	
	if(bmi160_get_status() != BMI160_OK) return BMI160_ERROR;
	
	data_config.slave_address  = BMI160_I2C_ADDR;
	data_config.send_data 	   = &reg_addr;
	data_config.send_length    = 1;
	data_config.receive_buffer = pData;
	data_config.receive_length = 6;
	
	//bmi160_i2c_init();
	i2c_status = hal_i2c_master_send_to_receive_polling(BMI160_I2C_NUM, &data_config);	
	//bmi160_app_i2c_deinit();
	
	if(HAL_I2C_STATUS_OK == i2c_status) 
	{
        bmi160_error_times = 0;
        return BMI160_OK;
    }
    bmi160_i2c_error_deal();     
    return BMI160_ERROR;
}
/*-------------------------------------------------------------------------------------------------
  功能:  初始化BMI160传感器
  输入： pI2cHandle--I2C句柄，确定BMI160连接的STM32哪个I2C接口
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
int8_t bmi160_init(void)
{         
    struct bmi160_sensor_data  bmi160_accel;
    struct bmi160_sensor_data  bmi160_gyro;        
    int8_t rslt = BMI160_OK;
    uint8_t i,error_code;
    
    //初始化读写函数
    bmi160_sensor.id = BMI160_I2C_ADDR;        		//I2C地址
    bmi160_sensor.interface = BMI160_I2C_INTF;      //接口，选择I2C
    bmi160_sensor.read = bmi160_i2c_bus_read;       //确定I2C读函数
    bmi160_sensor.write = bmi160_i2c_bus_write;     //确定I2C写函数
    bmi160_sensor.delay_ms = bmi160_delay_ms;       //确定延时函数
    
	//bmi160_i2c_init();		
    //初始化，3次
    for(i=0; i<3; i++)
    {
        rslt = bmi160_init2(&bmi160_sensor);             //bmi160初始化			
        if((bmi160_sensor.chip_id == BMI160_CHIP_ID) || (bmi160_sensor.chip_id == BMX160_CHIP_ID))
        {
            log_hal_info("bmi160  connect success! \r\n"); 
        }
        else
        {
            log_hal_info("bmi160  connect fail! \r\n"); 
            return BMI160_ERROR;
        }    
        //参数配置
        rslt += bmi160_app_para_set();	

        //中断配置
        rslt += bmi160_app_int_set();	
        
        //错误检测
        rslt += bmi160_get_error_code(&error_code);     
   
        rslt += error_code;      
		
		//bmi160_check_reg();
        
        //读取数据
        rslt += bmi160_get_sensor_data((BMI160_ACCEL_SEL | BMI160_GYRO_SEL), &bmi160_accel, &bmi160_gyro, &bmi160_sensor);
        	log_hal_info("bmi160_get_error_code %d \r\n",rslt); 

        //初始化成功
        if(rslt == 0) 
		{
			log_hal_info("bmi160 set para success! \r\n"); 
			break;
		}
        
    }
	//bmi160_i2c_deinit();
	
	return rslt;
}

/*-------------------------------------------------------------------------------------------------
  功能:  关闭BMI160传感器
  输入： pI2cHandle--I2C句柄，确定BMI160连接的STM32哪个I2C接口
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
int8_t bmi160_deinit(void)
{
	int8_t rslt = BMI160_OK;
	//设置加速度采样率、量程、带宽和电源模式--休眠
    bmi160_sensor.accel_cfg.odr = BMI160_ACCEL_ODR_25HZ;
    bmi160_sensor.accel_cfg.range = BMI160_ACCEL_RANGE_2G;
    bmi160_sensor.accel_cfg.bw = BMI160_ACCEL_BW_NORMAL_AVG4;
    bmi160_sensor.accel_cfg.power = BMI160_ACCEL_SUSPEND_MODE; 

    //设置角速度采样率、量程、带宽和电源模式--休眠
    bmi160_sensor.gyro_cfg.odr = BMI160_GYRO_ODR_25HZ;
    bmi160_sensor.gyro_cfg.range = BMI160_GYRO_RANGE_2000_DPS;
    bmi160_sensor.gyro_cfg.bw = BMI160_GYRO_BW_NORMAL_MODE;
    bmi160_sensor.gyro_cfg.power = BMI160_GYRO_SUSPEND_MODE; 

    //将配置写入BMI160
    rslt = bmi160_set_sens_conf(&bmi160_sensor);
	
	//关闭计步功能
	bmi160_set_step_counter(BMI160_DISABLE,&bmi160_sensor);
	return rslt;
}


int8_t bmi160_reset(void)
{
    bmi160_deinit();
    return bmi160_init();
}




/*-------------------------------------------------------------------------------------------------
  功能:  读取BMI160的通讯状态
  输入： 无
  返回： 0-连接正常  1-异常
--------------------------------------------------------------------------------------------------*/
int8_t bmi160_get_status(void)
{
    if(bmi160_error_times >= BMI160_ERROR_TIMES)  
    {
        return BMI160_ERROR;
    }
    return BMI160_OK;
}

/*-------------------------------------------------------------------------------------------------
  功能:  读取错误代码，用于调试阶段，正式后不用每次操作都读取
  输入： error_code--返回故障代码指针
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
int8_t bmi160_get_error_code(uint8_t *error_code)
{
     return(bmi160_get_regs(BMI160_ERROR_REG_ADDR, error_code, 1, &bmi160_sensor));   
}


/*-------------------------------------------------------------------------------------------------
  功能:  读取PMU状态信息
  输入： pmu_status--返回pmu状态指针
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
int8_t bmi160_get_pmu_status(uint8_t *pmu_status)
{
     return(bmi160_get_regs(BMI160_PMU_STATUS_ADDR, pmu_status, 1, &bmi160_sensor));   
}

/*-------------------------------------------------------------------------------------------------
  功能:  读取当前计步数
  输入： step_val--计步变量指针
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
int8_t bmi160_get_step(uint16_t *step_val)
{
    return(bmi160_read_step_counter(step_val, &bmi160_sensor));
}

/*-------------------------------------------------------------------------------------------------
  功能:  清除当前计步数
         发送0xB2到0x7E寄存器
  输入： 无
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
int8_t bmi160_clr_step(void)
{
	int8_t rslt = 0;
	uint8_t data =  BMI160_STEP_CLR_CMD;
	uint8_t reg_addr = BMI160_COMMAND_REG_ADDR;	
	rslt = bmi160_set_regs(reg_addr, &data, BMI160_ONE, &bmi160_sensor);
	return rslt;
   
}

/*-------------------------------------------------------------------------------------------------
  功能:  读取BMI160寄存器状态
  输入： 无
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
void bmi160_check_reg(void)
{
	uint16_t i = 0;
	uint8_t tmp;
	const uint8_t bmi160_reg[]={0x00,0x02,0x03,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,
								0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,
								0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x40,0x41,
								0x42,0x43,0x44,0x45,0x46,0x47,0x50,0x51,0x52,0x53,
								0x54,0x6c};
	log_hal_info("bmi160 reg value----\r\n",bmi160_reg[i],tmp);
    for(i=0; i<sizeof(bmi160_reg); i++)
    {
        bmi160_get_regs(bmi160_reg[i], &tmp, 1, &bmi160_sensor);
        log_hal_info("%2x---%4x\r\n",bmi160_reg[i],tmp);
    }
}

//=================================================================================================
//私有函数
/*-------------------------------------------------------------------------------------------------
  功能:  初始化BMI160传感器采样率和工作模式
  输入： 无
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
static int8_t bmi160_app_para_set(void)
{
    int8_t rslt = BMI160_OK;	
    //设置加速度采样率、量程、带宽和电源模式
    bmi160_sensor.accel_cfg.odr = BMI160_ACCEL_ODR_25HZ;
    bmi160_sensor.accel_cfg.range = BMI160_ACCEL_RANGE_2G;
    bmi160_sensor.accel_cfg.bw = BMI160_ACCEL_BW_NORMAL_AVG4;
    bmi160_sensor.accel_cfg.power = BMI160_ACCEL_LOWPOWER_MODE; 

    //设置角速度采样率、量程、带宽和电源模式
    bmi160_sensor.gyro_cfg.odr = BMI160_GYRO_ODR_25HZ;
    bmi160_sensor.gyro_cfg.range = BMI160_GYRO_RANGE_2000_DPS;
    bmi160_sensor.gyro_cfg.bw = BMI160_GYRO_BW_NORMAL_MODE;
    bmi160_sensor.gyro_cfg.power = BMI160_GYRO_NORMAL_MODE; 

    //将配置写入BMI160
    rslt = bmi160_set_sens_conf(&bmi160_sensor);
	
	//开启计步功能
	bmi160_set_step_counter(BMI160_ENABLE,&bmi160_sensor);
    
    return rslt;
}

/*-------------------------------------------------------------------------------------------------
  功能:  初始化BMI160传感器中断模式
  输入： 无
  返回： 0-成功  其它－失败
--------------------------------------------------------------------------------------------------*/
static int8_t bmi160_app_int_set(void)
{
    int8_t rslt = BMI160_OK;
//    uint8_t SetData;
    struct bmi160_int_settg int_config;

    //配置中断脚1为数据就绪中断，设置相关中断参数
    int_config.int_channel = BMI160_INT_CHANNEL_1; 
    int_config.int_type = BMI160_ACC_GYRO_DATA_RDY_INT;             
    int_config.int_pin_settg.output_en   = BMI160_ENABLE;           // 使能中断输出     
    int_config.int_pin_settg.output_mode = BMI160_DISABLE;          // 选择推拉模式     
    int_config.int_pin_settg.output_type = BMI160_ENABLE;           // 设置高电平中断
    int_config.int_pin_settg.edge_ctrl   = BMI160_ENABLE;           // 设置边沿触发中断
    int_config.int_pin_settg.input_en    = BMI160_DISABLE;          // 中断引脚不做输入
    int_config.int_pin_settg.latch_dur   = BMI160_LATCH_DUR_NONE;   // 没有锁存延时      
    rslt +=  bmi160_set_int_config(&int_config, &bmi160_sensor);    //写参数到BMI160

    //配置中断脚2，移动唤醒陀螺仪，不动时休眠陀螺仪
//    int_config.int_channel = BMI160_INT_CHANNEL_2;  
//    int_config.int_pin_settg.output_en   = BMI160_DISABLE;          // 不输出中断到int2引脚，这个不输出以下参数都无意义
//    int_config.int_pin_settg.output_mode = BMI160_DISABLE;          //  
//    int_config.int_pin_settg.output_type = BMI160_DISABLE;          //  
//    int_config.int_pin_settg.edge_ctrl   = BMI160_DISABLE;          //  
//    int_config.int_pin_settg.input_en    = BMI160_DISABLE;          //  
//    int_config.int_pin_settg.latch_dur   = BMI160_LATCH_DUR_NONE;   //    
    
    //配置加速度动作时，唤醒陀螺仪
//    int_config.int_type = BMI160_ACC_ANY_MOTION_INT;                                //类型动作中断    
//    int_config.int_type_cfg.acc_any_motion_int.anymotion_en= BMI160_ENABLE;         //开启动作中断
//    int_config.int_type_cfg.acc_any_motion_int.anymotion_x = BMI160_ENABLE;         //打开X轴动作中断
//    int_config.int_type_cfg.acc_any_motion_int.anymotion_y = BMI160_ENABLE;         //打开Y轴动作中断
//    int_config.int_type_cfg.acc_any_motion_int.anymotion_z = BMI160_ENABLE;         //打开z轴动作中断
//    int_config.int_type_cfg.acc_any_motion_int.anymotion_dur = ANYMOTION_SET_DUR;   //设置动作持续点数
//    int_config.int_type_cfg.acc_any_motion_int.anymotion_thr = ANYMOTION_SET_THR;   //设置动作阈值 
//    rslt +=  bmi160_set_int_config(&int_config, &bmi160_sensor);                    //写参数到BMI160   
        
    //配置加速度静止时，休眠陀螺仪
//    int_config.int_type = BMI160_ACC_SLOW_NO_MOTION_INT;                            //静止中断                           
//    int_config.int_type_cfg.acc_no_motion_int.no_motion_x   =  BMI160_ENABLE;       //打开X轴静止中断
//    int_config.int_type_cfg.acc_no_motion_int.no_motion_y   =  BMI160_ENABLE;       //打开X轴静止中断
//    int_config.int_type_cfg.acc_no_motion_int.no_motion_z   =  BMI160_ENABLE;       //打开X轴静止中断    
//    int_config.int_type_cfg.acc_no_motion_int.no_motion_sel =  BMI160_ENABLE;       //选择静止中断
//    int_config.int_type_cfg.acc_no_motion_int.no_motion_src =  BMI160_DISABLE;      //选择滤波后数据
//    int_config.int_type_cfg.acc_no_motion_int.no_motion_dur =  NOMOTION_SET_DUR;    //选择静止时间
//    int_config.int_type_cfg.acc_no_motion_int.no_motion_thres = NOMOTION_SET_THR;   //选择静止阈值   
//    rslt +=  bmi160_set_int_config(&int_config, &bmi160_sensor);  
        
    //设置陀螺仪电源模式由加速度运动唤醒，静止休眠
//    SetData = PMU_TRIGGER_SET;
//    rslt +=  bmi160_set_regs(BMI160_PMU_TRIGGER_ADDR, &SetData, 1, &bmi160_sensor);
   
   return rslt;
}

/*-------------------------------------------------------------------------------------------------
  功能:  延时，单位为ms
  输入： msec----需要延时单位
  返回： 无
--------------------------------------------------------------------------------------------------*/
static void bmi160_delay_ms(uint32_t time)
{   
//    #ifdef CTP_USED_TASK_DEALY
//		vTaskDelay(time / portTICK_PERIOD_MS);
//	#else
		hal_gpt_delay_ms(time);
//	#endif
}

/*-------------------------------------------------------------------------------------------------
  功能:  I2C 错误处理
  输入： 无
  返回： 无
--------------------------------------------------------------------------------------------------*/
static void bmi160_i2c_error_deal(void)
{
    if(bmi160_error_times < BMI160_ERROR_TIMES)  
    {
        //10次错误以上就不进行错误处理了，说明硬件有问题
        bmi160_error_times++;    
        log_hal_info("bmi160 i2c error \r\n");     
    }
}


/*-------------------------------------------------------------------------------------------------
  功能:  I2C总线读程序  
  输入： dev_addr---BMI160 I2C地址
         reg_addr---BMI160 寄存器地址
         reg_data---BMI160 寄存器读取返回数据指针
         cnt     ---BMI160 读取长度
  返回： 返回0-成功  -1 --失败
--------------------------------------------------------------------------------------------------*/
static int8_t bmi160_i2c_bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t cnt)
{	   
    uint8_t i;   
	hal_i2c_status_t i2c_status;	
	hal_i2c_send_to_receive_config_t data_config;	
    
    if(bmi160_get_status() != BMI160_OK) return BMI160_ERROR;
	
	data_config.slave_address  = dev_addr;
	data_config.send_data 	   = &reg_addr;
	data_config.send_length    = 1;
	data_config.receive_buffer = reg_data;
	data_config.receive_length = cnt;		
    
    for(i=0; i<3; i++)
    {     
		//bmi160_app_i2c_init();
		i2c_status = hal_i2c_master_send_to_receive_polling (BMI160_I2C_NUM, &data_config);	
		//bmi160_app_i2c_deinit(); 
        if(HAL_I2C_STATUS_OK == i2c_status) 
        {                
            bmi160_error_times = 0;
            return BMI160_OK;
        }  
        bmi160_delay_ms(1);
    }    
    bmi160_i2c_error_deal();    
    
	return BMI160_ERROR;
}

/*-------------------------------------------------------------------------------------------------
  功能:  I2C总线写程序  
  输入： dev_addr---BMI160 I2C地址
         reg_addr---BMI160 寄存器地址
         reg_data---BMI160 寄存器写数据指针
         cnt     ---BMI160 读取长度
  返回： 返回0-成功  -1 --失败
--------------------------------------------------------------------------------------------------*/
static int8_t bmi160_i2c_bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t cnt)
{
    uint8_t i;		
	uint8_t send_data[8];
	hal_i2c_status_t i2c_status;	
	
	if(bmi160_get_status() != BMI160_OK) return BMI160_ERROR;
	if(cnt>7) return BMI160_ERROR;
	
	send_data[0] = reg_addr;
	for(i=0; i<cnt; i++)
	{
		send_data[i+1] = *(reg_data++);
	}      
    for(i=0; i<3; i++)
    {
		//bmi160_app_i2c_init();
		i2c_status = hal_i2c_master_send_polling (BMI160_I2C_NUM, dev_addr, send_data, cnt+1);
		//bmi160_app_i2c_deinit();
        if(HAL_I2C_STATUS_OK == i2c_status)    
        {                
            bmi160_error_times = 0;
            return BMI160_OK;
        }  
        //bmi160_delay_ms(1);        
       
    }
    bmi160_i2c_error_deal();  	
    return BMI160_ERROR;
}




