#include "drv/e2p.h"
#include "drv/i2c.h"
#include "drv/delay.h"


#define E2P_ADDR            0xA0
#define MEMADD_SIZE         I2C_MEMADD_SIZE_16BIT

#ifdef USE_24XX512
    #define PAGE_SIZE       128
#else   //24C02
    #define PAGE_SIZE       8
#endif

extern handle_t i2c0handle;

int e2p_init(void)
{
    return 0;
}


int e2p_read_byte(U32 addr, U8 *data)
{
    return i2c_mem_read(i2c0handle, E2P_ADDR, addr, 0, data, 1);
}


int e2p_write_byte(U32 addr, U8 *data)
{
    int r = i2c_mem_write(i2c0handle, E2P_ADDR, addr, 0, data, 1);
    delay_ms(5);
    return r;
}


int e2p_read(U32 addr, U8 *data, U16 len)
{
    return i2c_mem_read(i2c0handle, E2P_ADDR, addr, 0, data, len);
}


int e2p_write(U32 addr, U8 *data, U16 len)
{
    int r;
    U16 i = 0;
    U16 cnt = 0;		//д���ֽڼ���
    
    if(0 == addr%PAGE_SIZE) {   //��ʼ��ַ�պ���ҳ��ʼ��ַ
        if(len <= PAGE_SIZE) {  //д����ֽ���������һҳ��ֱ��д��
            r = i2c_mem_write(i2c0handle, E2P_ADDR, addr, 0, data, len);
        }
        else {  //д����ֽ�������һҳ
            for(i = 0;i < len/PAGE_SIZE; i++) { //�Ƚ���ҳѭ��д��
                r = i2c_mem_write(i2c0handle, E2P_ADDR, addr+cnt, 0, data+cnt, PAGE_SIZE);
                cnt += PAGE_SIZE;
                delay_ms(5);
            }

            //��ʣ����ֽ�д��
            r = i2c_mem_write(i2c0handle, E2P_ADDR, addr+cnt, 0, data+cnt, len-cnt);
        }
    }
    else {   //��ʼ��ַƫ��ҳ��ʼ��ַ
        if(len <= (PAGE_SIZE - addr%PAGE_SIZE)) {    //�ڸ�ҳ����д��
            r = i2c_mem_write(i2c0handle, E2P_ADDR, addr, 0, data, len);
        }
        else {  //��ҳд����, �Ƚ���ҳд��
            cnt += PAGE_SIZE - addr%PAGE_SIZE;
            r = i2c_mem_write(i2c0handle, E2P_ADDR, addr+cnt, 0, data, cnt);
            delay_ms(5);

            addr += cnt;
            for(i=0; i<(len-cnt)/PAGE_SIZE; i++) {  //ѭ��д��ҳ����
                r = i2c_mem_write(i2c0handle, E2P_ADDR, addr+cnt, 0, data+cnt, PAGE_SIZE);
                delay_ms(5);
                cnt += PAGE_SIZE;
            }
            
            //��ʣ�µ��ֽ�д��
            r = i2c_mem_write(i2c0handle, E2P_ADDR, addr+cnt, 0, data+cnt, len-cnt);
        }
    }

    delay_ms(5);
    return r;
}


#if 0
typedef struct {
    U32 x[1000];
}test_t;
int r1,r2;
U8 tmp;
test_t tt;
void e2p_test(void)
{
    int i;
    for(i=0;i<1000;i++) {
        tt.x[i] = i;
    }
    r1 = e2p_write(0, (u8*)&tt, sizeof(tt));

    memset(&tt, 0, sizeof(tt));
    r2 = e2p_read(0, (u8*)&tt, sizeof(tt));
    delay_ms(100);
}

#endif

