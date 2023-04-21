#include "mb.h"
#include "modbus.h"
#include "task.h"


/*
离散量输入：主要用来读取单个位的数据，如IO的状态
线圈：开关输出信号，主要用来写入单个位的数据，与离散量构成组成对位的操作
输入寄存器：主要用来读取16位，也就是两个字节的数据
保持寄存器：主要用来写入16位的数据
DI：DigitalInput（数字输入，离散输入），一个地址一个数据位，用户只能读取它的状态，不能修改。
    以一个 bit表示 On/Off，用来记录控制信号的状态输入，例如：开关，接触点，马达运转，超限
    switch…等等。于PLC上被称为Input relay、input coil等。
DO：DigitalOutput（数字输出，线圈输出），一个地址一个数据位，用户可以置位、复位，可以回读状态。
    以一个 bit表示 On/Off，用来输出控制信号，以激活或停止马达，警铃，灯光…等等。于PLC上被称为
    Output relay、Output coil等。
AI：Analog Input（模拟输入，输入寄存器），一个地址16位数据，用户只能读，不能修改。
    以16 bits integer表示一个数值，用来记录控制信号的数值输入，例如：温度、流量、料量、速度、转速、
    文件板开度、液位、重量…等等。于PLC上被称为Input register。
AO：AnalogOutput（模拟输出，保持寄存器），一个地址16位数据，用户可以写，也可以回读。
    以16 bits integer表示一个数值，用来输出控制信号的数值，例如：温度、流量、速度、转速、文件板开度、
    饲料量…等等设定值。于PLC上被称为Output register、Holding register。



功能代码：  占用一个字节，取值范围为1~255，其中：
           1~127为功能代码编号, 其中65~72和100~110为用户自定义编码
           
           有效功能码有二十几种，但是一般使用上都以1、2、3、4、5、6、
           15、16等八种最为常用
           01H：读取线圈寄存器(Output Coil),位操作
           02H：读取离散输入寄存器(Input Relay),位操作
           03H：读保持寄存器(Holding Register),字节操作
           04H：读取输入寄存器(Input Register),字节操作
           05H：写单个线圈寄存器(Output Coil),位操作
           06H：写单个保持寄存器(Holding Register),字节
           0FH：写多个线圈寄存器(Output Coil),字节指令操作,可写多个
           10H：写多个保持寄存器(Holding Register),字节指令操作,可写多个
           14H：读多个扩展寄存器(Extended Register),字节操作
           15H：写多个扩展寄存器(Extended Register),字节操作
           
           128~255为保留值，用于异常消息应答报文
           
报文种类：  ASCII(LRC校验)、RTU(CRC16校验)、TCP(CRC32校验)三种
报文格式：  地址+功能码+数据+校验
           地址为8位，0地址用于广播，
*/




typedef struct {
    modbusHandler_t handle;
    U16             data[8];
}mb_data_t;

typedef struct {
    mb_data_t host;
    mb_data_t slave;
}mb_handle_t;


static mb_handle_t mbHandle={0};

static void mbus_task(void *arg)
{
    while(1) {
        
    }
}
static void mbus_task_start(void *arg)
{
#ifdef OS_KERNEL
    osThreadAttr_t  attr={0};
    
    attr.name = "mbus";
    attr.stack_size = 1024;
    attr.priority = osPriorityNormal;
    osThreadNew(mbus_task, arg, &attr);
#endif
}
static void mbus_task_stop(void)
{
    
}


int mbus_init(mb_cfg_t *master, mb_cfg_t *slave)
{
    mb_data_t *m=&mbHandle.master;
    
    if(master) {
        mb_data_t *m=&mbHandle.master;
        
        m->handle.uModbusType = MB_SLAVE;
        //m->handle.port =  &huart3;
        m->handle.u8id = 1; //slave ID
        m->handle.u16timeOut = 1000;
        m->handle.EN_Port = NULL; // No RS485
        //m->handle.EN_Pin = LD2_Pin; // RS485 Enable
        m->handle.u16regs = m->data;
        m->handle.u16regsize= sizeof(m->data)/sizeof(m->data[0]);
        m->handle.xTypeHW = USART_HW;

        ModbusInit(&m->handle);
        ModbusStart(&m->handle);
    }
    
    if(slave) {
        mb_data_t *s=&mbHandle.slave;
        
        s->handle.uModbusType = MB_SLAVE;
        //s->handle.port =  &huart3;
        s->handle.u8id = 1; //slave ID
        s->handle.u16timeOut = 1000;
        s->handle.EN_Port = NULL; // No RS485
         //s->handle.EN_Pin = LD2_Pin; // RS485 Enable
        s->handle.u16regs = s->data;
        s->handle.u16regsize= sizeof(s->data)/sizeof(s->data[0]);
        s->handle.xTypeHW = USART_HW;

        ModbusInit(&s->handle);
        ModbusStart(&s->handle);
    }
    
    return 0;
}










