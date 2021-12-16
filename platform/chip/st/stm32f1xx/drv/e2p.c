#include "drv/e2p.h"
#include "drv/i2c.h"
#include "drv/delay.h"
#include "lock.h"


#define E2P_ADDR            0xA0
#define MEMADD_SIZE         I2C_MEMADD_SIZE_16BIT

#ifdef USE_24XX512
    #define PAGE_SIZE       128
#else   //24C02
    #define PAGE_SIZE       8
#endif


extern handle_t i2cHandle;


int e2p_init(void)
{
    return 0;
}


int e2p_read_byte(U32 addr, U8 *data)
{
    return i2c_mem_read(i2cHandle, E2P_ADDR, addr, MEMADD_SIZE, data, 1);
}


int e2p_write_byte(U32 addr, U8 *data)
{
    int r = i2c_mem_write(i2cHandle, E2P_ADDR, addr, MEMADD_SIZE, data, 1);
    delay_ms(5);
    return r;
}


int e2p_read(U32 addr, U8 *data, U32 len)
{
    return i2c_mem_read(i2cHandle, E2P_ADDR, addr, MEMADD_SIZE, data, len);
}


int e2p_write(U32 addr, U8 *data, U32 len)
{
    int r;
    U16 i = 0;
    U16 cnt = 0;		//写入字节计数
    
    if(0 == addr%PAGE_SIZE) {   //起始地址刚好是页开始地址
        if(len <= PAGE_SIZE) {  //写入的字节数不大于一页，直接写入
            r = i2c_mem_write(i2cHandle, E2P_ADDR, addr, MEMADD_SIZE, data, len);
        }
        else {  //写入的字节数大于一页
            for(i = 0;i < len/PAGE_SIZE; i++) { //先将整页循环写入
                r = i2c_mem_write(i2cHandle, E2P_ADDR, addr+cnt, MEMADD_SIZE, data+cnt, PAGE_SIZE);
                cnt += PAGE_SIZE;
                delay_ms(5);
            }

            //将剩余的字节写入
            r = i2c_mem_write(i2cHandle, E2P_ADDR, addr+cnt, MEMADD_SIZE, data+cnt, len-cnt);
        }
    }
    else {   //起始地址偏离页开始地址
        if(len <= (PAGE_SIZE - addr%PAGE_SIZE)) {    //在该页可以写完
            r = i2c_mem_write(i2cHandle, E2P_ADDR, addr, MEMADD_SIZE, data, len);
        }
        else {  //该页写不完, 先将该页写完
            cnt += PAGE_SIZE - addr%PAGE_SIZE;
            r = i2c_mem_write(i2cHandle, E2P_ADDR, addr+cnt, MEMADD_SIZE, data, cnt);
            delay_ms(5);

            addr += cnt;
            for(i=0; i<(len-cnt)/PAGE_SIZE; i++) {  //循环写整页数据
                r = i2c_mem_write(i2cHandle, E2P_ADDR, addr+cnt, MEMADD_SIZE, data+cnt, PAGE_SIZE);
                delay_ms(5);
                cnt += PAGE_SIZE;
            }
            
            //将剩下的字节写入
            r = i2c_mem_write(i2cHandle, E2P_ADDR, addr+cnt, MEMADD_SIZE, data+cnt, len-cnt);
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

