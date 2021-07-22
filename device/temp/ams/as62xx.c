#include "drv/i2c.h"
#include "drv/delay.h"
#include "temp/ams/as62xx.h"



#define AS_ADDR         (0x48)
#define TEMP_LSB        (0.0078125F)
#define TEMP_LOW        (-10.0F)
#define TEMP_HIGH       (70.0F)


static handle_t asHandle=NULL;

//////////////////////////////////////////////////////////
static int as_read(U8 *data, U16 len)
{
    return  i2c_read(asHandle, AS_ADDR, data, len, 1);
}
static int as_write(U8 *data, U16 len)
{
    return i2c_write(asHandle, AS_ADDR, data, len, 1);
}

static int reg_read(U8 reg, U16 *data)
{
    int r;
    U8  tmp[2];
    
    r = as_write(&reg, 1);
    if(r==0) {
        r = as_read(tmp, 2);
        *data = (tmp[0]<<8)|tmp[1];
    }
    
    return r;
}
static int reg_write(U8 reg, U16 data)
{
    U8 tmp[3] = {reg, (U8)(data >> 8), (U8)data};
    
    return as_write(tmp, 3); 
}
/////////////////////////////////////////////////////////

int as62xx_init(handle_t h)
{
    int r;
    U16 tmp=0x40a0,tmp2=0;
    S16 temp;
    as62xx_config_t cfg = {
        .cr = AS62XX_CONV_RATE025,
        .state = AS62XX_STATE_ACTIVE,
        .alert_mode = AS62XX_ALERT_INTERRUPT,
        .alert_polarity = AS62XX_ALERT_ACTIVE_LOW,
        .cf = AS62XX_CONSEC_FAULTS2,
        .singleShot = 0,
    };
    
    asHandle = h;
    
    tmp |= cfg.cr | cfg.alert_mode | cfg.alert_polarity | cfg.cf | cfg.state;
    if(cfg.singleShot){
        tmp |= AS62XX_SINGLE_SHOT;
    }
    
    r = reg_write((U8)AS62XX_REG_CONFIG, tmp);
    if(r) return r;
    
    reg_read((U8)AS62XX_REG_CONFIG, &tmp2);
    if(r) return r;
    
    temp = TEMP_LOW/TEMP_LSB;
    r = reg_write((U8)AS62XX_REG_TLOW, temp);
    if(r) return r;
    
    temp = TEMP_HIGH/TEMP_LSB;
    r = reg_write((U8)AS62XX_REG_THIGH, temp);
    if(r) return r;

    //as62xx_test();
    
    return r;
}


int as62xx_test(void)
{
    static F32 temp=0.0;
    
    while(1) {
        as62xx_get(&temp);
        delay_ms(300);
    }
}



int as62xx_get(F32 *temp)
{
    F32 t;
    S16 tmp=0;
    
    reg_read((U8)AS62XX_REG_TVAL, (U16*)&tmp);
    t = tmp*TEMP_LSB;
    
    if(temp) *temp = t;
    
    return 0;
}



