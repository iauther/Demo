#include "bmp280/bmp280.h"
#include "drv/delay.h"
#include "drv/i2c.h"
#include "log.h"
#include "myCfg.h"



/*
 *  @return Status of execution
 *  @retval 0 -> Success
 *  @retval >0 -> Failure Info
 */
extern handle_t i2c1Handle;
static handle_t bmpHandle=NULL;



static int8_t i2c_reg_read(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t length)
{
    i2c_write(bmpHandle, i2c_addr, &reg_addr, 1, 1, length);
    return i2c_read(bmpHandle, i2c_addr, reg_data, length, 1, length);
}
static int8_t i2c_reg_write(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t length)
{
    U8 tmp[length+1];
    
    tmp[0] = reg_addr;
    memcpy(tmp+1, reg_data, length);
    return i2c_write(bmpHandle, i2c_addr, tmp, length+1, 1, length);
}


static void print_err(int8_t err)
{
    if (err != BMP280_OK) {
        if (err == BMP280_E_NULL_PTR) {
            LOGE("Error [%d] : Null pointer error\r\n", err);
        }
        else if (err == BMP280_E_COMM_FAIL) {
            LOGE("Error [%d] : Bus communication failed\r\n", err);
        }
        else if (err == BMP280_E_IMPLAUS_TEMP) {
            LOGE("Error [%d] : Invalid Temperature\r\n", err);
        }
        else if (err == BMP280_E_DEV_NOT_FOUND) {
            LOGE("Error [%d] : Device not found\r\n", err);
        }
        else {
            /* For more error codes refer "*_defs.h" */
            LOGE("Error [%d] : Unknown error code\r\n", err);
        }
    }
}



static struct bmp280_dev bmp280;
int bmp280_init(void)
{
    int8_t r;
    bmp280_t b;
    struct bmp280_config conf;

    bmpHandle = i2c1Handle;
    
    bmp280.delay_ms = delay_ms;

    /* Assign device I2C address based on the status of SDO pin (GND for PRIMARY(0x76) & VDD for SECONDARY(0x77)) */
    bmp280.dev_id = BMP280_ADDR;
    bmp280.intf = BMP280_I2C_INTF;
    bmp280.read = i2c_reg_read;
    bmp280.write = i2c_reg_write;

    r = bmp280_init2(&bmp280);

    //Always read the current settings before writing, especially when all the configuration is not modified
    r = bmp280_get_config(&conf, &bmp280);

    /* configuring the temperature oversampling, filter coefficient and output data rate */
    /* Overwrite the desired settings */
    conf.filter = BMP280_FILTER_COEFF_2;

    /* Pressure oversampling set at 4x */
    conf.os_pres = BMP280_OS_4X;
    conf.os_temp = BMP280_OS_1X;

    // Setting the output data rate
    conf.odr = BMP280_ODR_0_5_MS;//BMP280_ODR_1000_MS;
    r = bmp280_set_config(&conf, &bmp280);

    /* Always set the power mode after setting the configuration */
    r = bmp280_set_power_mode(BMP280_FORCED_MODE, &bmp280);
    
    if(r==BMP280_OK) {
        bmp280_get(&b);
    }

    return 0;
}


int bmp280_deinit(void)
{
    return 0;
}


int bmp280_get(bmp280_t *bmp)
{
    struct bmp280_uncomp_data ucomp_data;
    
    if(!bmp) {
        return -1;
    }
    
    bmp280_get_uncomp_data(&ucomp_data, &bmp280);
    bmp280_get_comp_pres_float(&bmp->pres, ucomp_data.uncomp_press, &bmp280);
    bmp280_get_comp_temp_float(&bmp->temp, ucomp_data.uncomp_temp,  &bmp280);
    bmp->pres /= 1000.0F;
        
    return 0;
}


