#include "yaoxy.h"
#include "drv/spi.h"
#include "drv/gpio.h"
#include "drv/delay.h"

extern handle_t spi1Handle;
static handle_t yaoHandle=NULL;

static void yaoxy_write_byte(U8 id, U8 data)
{
	U8 tmp=0;

	tmp = 0xF8|(id<<1);
	spi_write(yaoHandle, &tmp, 1, 100);
	
	tmp = data&0xF0;
	spi_write(yaoHandle, &tmp, 1, 100);
	
	tmp = (data<<4)&0xF0;
	spi_write(yaoHandle, &tmp, 1, 100);
	delay_ms(1);
}




int yaoxy_init(void)
{
    yaoHandle = spi1Handle;
    
    yaoxy_write_byte(YAOXY_CMD, 0x30);  //选择基本指令操作
	yaoxy_write_byte(YAOXY_CMD, 0x06);
	yaoxy_write_byte(YAOXY_CMD, 0x0c);  //显示开关光标
	yaoxy_write_byte(YAOXY_CMD, 0x01);  //清除显示内容
	yaoxy_write_byte(YAOXY_CMD, 0x80);	 //从0x80地址开始显示
    
    return 0;
}


int yaoxy_write(U16 x, U16 y, U8 *data)
{
    int i;
    
    if(!data) {
        return -1;
    }
    
    yaoxy_write_byte(YAOXY_CMD, 0x80+y*16+x);
    for(i=0 ;; i++) {
        if(data[i]) {
            yaoxy_write_byte(YAOXY_DAT, data[i]);
            
            if(data[i+1]==0)
                break;
        
            yaoxy_write_byte(YAOXY_DAT, data[i+1]);
        }
        else {
            yaoxy_write_byte(YAOXY_DAT, data[i+1]+0x30);
        }
    }
    
    return 0;
}










