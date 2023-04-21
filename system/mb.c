#include "mb.h"
#include "modbus.h"
#include "task.h"


/*
��ɢ�����룺��Ҫ������ȡ����λ�����ݣ���IO��״̬
��Ȧ����������źţ���Ҫ����д�뵥��λ�����ݣ�����ɢ��������ɶ�λ�Ĳ���
����Ĵ�������Ҫ������ȡ16λ��Ҳ���������ֽڵ�����
���ּĴ�������Ҫ����д��16λ������
DI��DigitalInput���������룬��ɢ���룩��һ����ַһ������λ���û�ֻ�ܶ�ȡ����״̬�������޸ġ�
    ��һ�� bit��ʾ On/Off��������¼�����źŵ�״̬���룬���磺���أ��Ӵ��㣬�����ת������
    switch���ȵȡ���PLC�ϱ���ΪInput relay��input coil�ȡ�
DO��DigitalOutput�������������Ȧ�������һ����ַһ������λ���û�������λ����λ�����Իض�״̬��
    ��һ�� bit��ʾ On/Off��������������źţ��Լ����ֹͣ�����壬�ƹ⡭�ȵȡ���PLC�ϱ���Ϊ
    Output relay��Output coil�ȡ�
AI��Analog Input��ģ�����룬����Ĵ�������һ����ַ16λ���ݣ��û�ֻ�ܶ��������޸ġ�
    ��16 bits integer��ʾһ����ֵ��������¼�����źŵ���ֵ���룬���磺�¶ȡ��������������ٶȡ�ת�١�
    �ļ��忪�ȡ�Һλ���������ȵȡ���PLC�ϱ���ΪInput register��
AO��AnalogOutput��ģ����������ּĴ�������һ����ַ16λ���ݣ��û�����д��Ҳ���Իض���
    ��16 bits integer��ʾһ����ֵ��������������źŵ���ֵ�����磺�¶ȡ��������ٶȡ�ת�١��ļ��忪�ȡ�
    ���������ȵ��趨ֵ����PLC�ϱ���ΪOutput register��Holding register��



���ܴ��룺  ռ��һ���ֽڣ�ȡֵ��ΧΪ1~255�����У�
           1~127Ϊ���ܴ�����, ����65~72��100~110Ϊ�û��Զ������
           
           ��Ч�������ж�ʮ���֣�����һ��ʹ���϶���1��2��3��4��5��6��
           15��16�Ȱ�����Ϊ����
           01H����ȡ��Ȧ�Ĵ���(Output Coil),λ����
           02H����ȡ��ɢ����Ĵ���(Input Relay),λ����
           03H�������ּĴ���(Holding Register),�ֽڲ���
           04H����ȡ����Ĵ���(Input Register),�ֽڲ���
           05H��д������Ȧ�Ĵ���(Output Coil),λ����
           06H��д�������ּĴ���(Holding Register),�ֽ�
           0FH��д�����Ȧ�Ĵ���(Output Coil),�ֽ�ָ�����,��д���
           10H��д������ּĴ���(Holding Register),�ֽ�ָ�����,��д���
           14H���������չ�Ĵ���(Extended Register),�ֽڲ���
           15H��д�����չ�Ĵ���(Extended Register),�ֽڲ���
           
           128~255Ϊ����ֵ�������쳣��ϢӦ����
           
�������ࣺ  ASCII(LRCУ��)��RTU(CRC16У��)��TCP(CRC32У��)����
���ĸ�ʽ��  ��ַ+������+����+У��
           ��ַΪ8λ��0��ַ���ڹ㲥��
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










