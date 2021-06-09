#include <stdint.h>
#include <math.h>
#include <errno.h>
#include "drv/delay.h"
#include "drv/i2c.h"
#include "drv/gpio.h"
#include "temp/mlx/mlx90632.h"
#include "temp/mlx/mlx90632_extended_meas.h"


#define MLX_ADDR    0x3A
#define POW10 10000000000LL


extern handle_t i2c2Handle;
static handle_t mlxHandle=NULL;

#if defined(__CC_ARM)
#pragma diag_suppress 177
#endif



int32_t mlx_read(int16_t reg, uint16_t *data)
{
    int r;
    U8 tmp[2];
    
    r = i2c_mem_read(mlxHandle, MLX_ADDR, reg, 2, tmp, sizeof(tmp));
    if(r==0) {
        *data = tmp[1]|(tmp[0]<<8);
    }
    
    return 0;
}
int32_t mlx_write(int16_t reg, uint16_t data)
{
    int r;
    U8 tmp[3];
    
    tmp[0] = reg;
    tmp[1] = data>>8;
    tmp[2] = data;
    r = i2c_mem_write(mlxHandle, MLX_ADDR, reg, 2, tmp, sizeof(tmp));
    
    return r;
}
int mlx_read32(int16_t reg, uint32_t *value)
{
    int r;
	U8  tmp[4];
    
	r = i2c_mem_read(mlxHandle, MLX_ADDR, reg, 2, tmp, sizeof(tmp));
	if(r==0) {
        *value = tmp[2]<<24|tmp[3]<<16|tmp[0]<<8|tmp[1];
    }
	return r;
}
static int mlx_e2p_read(int32_t *PR, int32_t *PG, int32_t *PO, int32_t *PT, int32_t *Ea, int32_t *Eb, int32_t *Fa, int32_t *Fb, int32_t *Ga, int16_t *Gb, int16_t *Ha, int16_t *Hb, int16_t *Ka)
{
	int ret;
	ret = mlx_read32(MLX90632_EE_P_R, (uint32_t *) PR);
	if(ret < 0)
		return ret;
	ret = mlx_read32(MLX90632_EE_P_G, (uint32_t *) PG);
	if(ret < 0)
		return ret;
	ret = mlx_read32(MLX90632_EE_P_O, (uint32_t *) PO);
	if(ret < 0)
		return ret;
	ret = mlx_read32(MLX90632_EE_P_T, (uint32_t *) PT);
	if(ret < 0)
		return ret;
	ret = mlx_read32(MLX90632_EE_Ea, (uint32_t *) Ea);
	if(ret < 0)
		return ret;
	ret = mlx_read32(MLX90632_EE_Eb, (uint32_t *) Eb);
	if(ret < 0)
		return ret;
	ret = mlx_read32(MLX90632_EE_Fa, (uint32_t *) Fa);
	if(ret < 0)
		return ret;
	ret = mlx_read32(MLX90632_EE_Fb, (uint32_t *) Fb);
	if(ret < 0)
		return ret;
	ret = mlx_read32(MLX90632_EE_Ga, (uint32_t *) Ga);
	if(ret < 0)
		return ret;
	ret = mlx_read(MLX90632_EE_Gb, (uint16_t *) Gb);
	if(ret < 0)
		return ret;
	ret = mlx_read(MLX90632_EE_Ha, (uint16_t *) Ha);
	if(ret < 0)
		return ret;
	ret = mlx_read(MLX90632_EE_Hb, (uint16_t *) Hb);
	if(ret < 0)
		return ret;
	ret = mlx_read(MLX90632_EE_Ka, (uint16_t *) Ka);
	if(ret < 0)
		return ret;
	return 0;
}

static void usleep(int min_range, int max_range)
{
    delay_us(max_range);
}
static void msleep(int msecs)
{
    delay_ms(msecs);
}
/////////////////////////////////////////////////////////////////////



int mlx90632_start_measurement(void)
{
   int ret, tries = MLX90632_MAX_NUMBER_MESUREMENT_READ_TRIES;
   uint16_t reg_status;
 
   ret = mlx_read(MLX90632_REG_STATUS, &reg_status);
   if (ret < 0)
       return ret;
 
   ret = mlx_write(MLX90632_REG_STATUS, reg_status & (~MLX90632_STAT_DATA_RDY));
   if (ret < 0)
       return ret;
   
   while (tries-- > 0)
   {
       ret = mlx_read(MLX90632_REG_STATUS, &reg_status);
       if (ret < 0)
           return ret;
       if (reg_status & MLX90632_STAT_DATA_RDY)
           break;
       /* minimum wait time to complete measurement
        * should be calculated according to refresh rate
        * atm 10ms - 11ms
        */
       usleep(10000, 11000);
   }
   
   if (tries < 0)
   {
       // data not ready
       return -ETIMEDOUT;
   }
   
   return (reg_status & MLX90632_STAT_CYCLE_POS) >> 2;
}
    
static int32_t mlx90632_channel_new_select(int32_t ret, uint8_t *channel_new, uint8_t *channel_old)
{
    switch (ret)
   {
       case 1:
           *channel_new = 1;
           *channel_old = 2;
           break;

       case 2:
           *channel_new = 2;
           *channel_old = 1;
           break;

       default:
           return -EINVAL;
   }
   return 0;
}

static int32_t mlx90632_read_temp_ambient_raw(int16_t *ambient_new_raw, int16_t *ambient_old_raw)
{
    int32_t ret;
    uint16_t read_tmp;

    ret = mlx_read(MLX90632_RAM_3(1), &read_tmp);
    if (ret < 0)
        return ret;
    *ambient_new_raw = (int16_t)read_tmp;

    ret = mlx_read(MLX90632_RAM_3(2), &read_tmp);
    if (ret < 0)
        return ret;
    *ambient_old_raw = (int16_t)read_tmp;

    return ret;
}

static int32_t mlx90632_read_temp_object_raw(int32_t start_measurement_ret,
                                             int16_t *object_new_raw, int16_t *object_old_raw)
{
    int32_t ret;
    uint16_t read_tmp;
    int16_t read;
    uint8_t channel, channel_old;

    ret = mlx90632_channel_new_select(start_measurement_ret, &channel, &channel_old);
    if (ret != 0)
        return -EINVAL;

    ret = mlx_read(MLX90632_RAM_2(channel), &read_tmp);
    if (ret < 0)
        return ret;

    read = (int16_t)read_tmp;

    ret = mlx_read(MLX90632_RAM_1(channel), &read_tmp);
    if (ret < 0)
        return ret;
    *object_new_raw = (read + (int16_t)read_tmp) / 2;

    ret = mlx_read(MLX90632_RAM_2(channel_old), &read_tmp);
    if (ret < 0)
        return ret;
    read = (int16_t)read_tmp;

    ret = mlx_read(MLX90632_RAM_1(channel_old), &read_tmp);
    if (ret < 0)
        return ret;
    *object_old_raw = (read + (int16_t)read_tmp) / 2;

    return ret;
}

static int32_t mlx90632_read_temp_raw(int16_t *ambient_new_raw, int16_t *ambient_old_raw,
                               int16_t *object_new_raw, int16_t *object_old_raw)
{
    int32_t ret, start_measurement_ret;

    // trigger and wait for measurement to complete
    start_measurement_ret = mlx90632_start_measurement();
    if (start_measurement_ret < 0)
        return start_measurement_ret;

    ret = mlx90632_read_temp_ambient_raw(ambient_new_raw, ambient_old_raw);
    if (ret < 0)
        return ret;

    ret = mlx90632_read_temp_object_raw(start_measurement_ret, object_new_raw, object_old_raw);

    return ret;
}

static int32_t mlx90632_read_temp_raw_burst(int16_t *ambient_new_raw, int16_t *ambient_old_raw,
                                     int16_t *object_new_raw, int16_t *object_old_raw)
{
    int32_t ret, start_measurement_ret;

    // trigger and wait for measurement to complete
    start_measurement_ret = mlx90632_start_measurement_burst();
    if (start_measurement_ret < 0)
        return start_measurement_ret;

    ret = mlx90632_read_temp_ambient_raw(ambient_new_raw, ambient_old_raw);
    if (ret < 0)
        return ret;

    ret = mlx90632_read_temp_object_raw(2, object_new_raw, object_old_raw);

    return ret;
}


/* DSPv5 */
static double mlx90632_preprocess_temp_ambient(int16_t ambient_new_raw, int16_t ambient_old_raw, int16_t Gb)
{
    double VR_Ta, kGb;

    kGb = ((double)Gb) / 1024.0;

    VR_Ta = ambient_old_raw + kGb * (ambient_new_raw / (MLX90632_REF_3));
    return ((ambient_new_raw / (MLX90632_REF_3)) / VR_Ta) * 524288.0;
}

static double mlx90632_preprocess_temp_object(int16_t object_new_raw, int16_t object_old_raw,
                                       int16_t ambient_new_raw, int16_t ambient_old_raw,
                                       int16_t Ka)
{
    double VR_IR, kKa;

    kKa = ((double)Ka) / 1024.0;

    VR_IR = ambient_old_raw + kKa * (ambient_new_raw / (MLX90632_REF_3));
    return ((((object_new_raw + object_old_raw) / 2) / (MLX90632_REF_12)) / VR_IR) * 524288.0;
}

static double mlx90632_calc_temp_ambient(int16_t ambient_new_raw, int16_t ambient_old_raw, int32_t P_T,
                                  int32_t P_R, int32_t P_G, int32_t P_O, int16_t Gb)
{
    double Asub, Bsub, Ablock, Bblock, Cblock, AMB;

    AMB = mlx90632_preprocess_temp_ambient(ambient_new_raw, ambient_old_raw, Gb);

    Asub = ((double)P_T) / (double)17592186044416.0;
    Bsub = (double)AMB - ((double)P_R / (double)256.0);
    Ablock = Asub * (Bsub * Bsub);
    Bblock = (Bsub / (double)P_G) * (double)1048576.0;
    Cblock = (double)P_O / (double)256.0;

    return Bblock + Ablock + Cblock;
}

static double mlx90632_calc_temp_object_iteration(double prev_object_temp, int32_t object, double TAdut,
                                                  int32_t Ga, int32_t Fa, int32_t Fb, int16_t Ha, int16_t Hb,
                                                  double emissivity)
{
    double calcedGa, calcedGb, calcedFa, TAdut4, first_sqrt;
    // temp variables
    double KsTAtmp, Alpha_corr;
    double Ha_customer, Hb_customer;


    Ha_customer = Ha / ((double)16384.0);
    Hb_customer = Hb / ((double)1024.0);
    calcedGa = ((double)Ga * (prev_object_temp - 25)) / ((double)68719476736.0);
    KsTAtmp = (double)Fb * (TAdut - 25);
    calcedGb = KsTAtmp / ((double)68719476736.0);
    Alpha_corr = (((double)(Fa * POW10)) * Ha_customer * (double)(1 + calcedGa + calcedGb)) /
                 ((double)70368744177664.0);
    calcedFa = object / (emissivity * (Alpha_corr / POW10));
    TAdut4 = (TAdut + 273.15) * (TAdut + 273.15) * (TAdut + 273.15) * (TAdut + 273.15);

    first_sqrt = sqrt(calcedFa + TAdut4);

    return sqrt(first_sqrt) - 273.15 - Hb_customer;
}

static double mlx90632_calc_temp_object_iteration_reflected(double prev_object_temp, int32_t object, double TAdut, double TaTr4,
                                                            int32_t Ga, int32_t Fa, int32_t Fb, int16_t Ha, int16_t Hb,
                                                            double emissivity)
{
    double calcedGa, calcedGb, calcedFa, first_sqrt;
    // temp variables
    double KsTAtmp, Alpha_corr;
    double Ha_customer, Hb_customer;

    Ha_customer = Ha / ((double)16384.0);
    Hb_customer = Hb / ((double)1024.0);
    calcedGa = ((double)Ga * (prev_object_temp - 25)) / ((double)68719476736.0);
    KsTAtmp = (double)Fb * (TAdut - 25);
    calcedGb = KsTAtmp / ((double)68719476736.0);
    Alpha_corr = (((double)(Fa * POW10)) * Ha_customer * (double)(1 + calcedGa + calcedGb)) /
                 ((double)70368744177664.0);
    calcedFa = object / (emissivity * (Alpha_corr / POW10));

    first_sqrt = sqrt(calcedFa + TaTr4);

    return sqrt(first_sqrt) - 273.15 - Hb_customer;
}

static double emissivity = 0.0;
static void mlx90632_set_emissivity(double value)
{
    emissivity = value;
}

double mlx90632_get_emissivity(void)
{
    if (emissivity == 0.0)
    {
        return 1.0;
    }
    else
    {
        return emissivity;
    }
}

static double mlx90632_calc_temp_object(int32_t object, int32_t ambient,
                                 int32_t Ea, int32_t Eb, int32_t Ga, int32_t Fa, int32_t Fb,
                                 int16_t Ha, int16_t Hb)
{
    double kEa, kEb, TAdut;
    double temp = 25.0;
    double tmp_emi = mlx90632_get_emissivity();
    int8_t i;

    kEa = ((double)Ea) / ((double)65536.0);
    kEb = ((double)Eb) / ((double)256.0);
    TAdut = (((double)ambient) - kEb) / kEa + 25;

    //iterate through calculations
    for (i = 0; i < 5; ++i)
    {
        temp = mlx90632_calc_temp_object_iteration(temp, object, TAdut, Ga, Fa, Fb, Ha, Hb, tmp_emi);
    }
    return temp;
}

static double mlx90632_calc_temp_object_reflected(int32_t object, int32_t ambient, double reflected,
                                           int32_t Ea, int32_t Eb, int32_t Ga, int32_t Fa, int32_t Fb,
                                           int16_t Ha, int16_t Hb)
{
    double kEa, kEb, TAdut;
    double temp = 25.0;
    double tmp_emi = mlx90632_get_emissivity();
    double TaTr4;
    double ta4;
    int8_t i;

    kEa = ((double)Ea) / ((double)65536.0);
    kEb = ((double)Eb) / ((double)256.0);
    TAdut = (((double)ambient) - kEb) / kEa + 25;

    TaTr4 = reflected + 273.15;
    TaTr4 = TaTr4 * TaTr4;
    TaTr4 = TaTr4 * TaTr4;
    ta4 = TAdut + 273.15;
    ta4 = ta4 * ta4;
    ta4 = ta4 * ta4;
    TaTr4 = TaTr4 - (TaTr4 - ta4) / tmp_emi;

    //iterate through calculations
    for (i = 0; i < 5; ++i)
    {
        temp = mlx90632_calc_temp_object_iteration_reflected(temp, object, TAdut, TaTr4, Ga, Fa, Fb, Ha, Hb, tmp_emi);
    }
    return temp;
}






int32_t mlx90632_addressed_reset(void)
{
    int32_t ret;

    ret = mlx_write(0x3005, MLX90632_RESET_CMD);
    if (ret < 0)
        return ret;

    usleep(150, 200);

    return 0;
}

static int32_t mlx90632_get_measurement_time(uint16_t meas)
{
    int32_t ret;
    uint16_t reg;

    ret = mlx_read(meas, &reg);
    if (ret < 0)
        return ret;

    reg &= MLX90632_EE_REFRESH_RATE_MASK;
    reg = reg >> 8;

    return MLX90632_MEAS_MAX_TIME >> reg;
}

static int32_t mlx90632_calculate_dataset_ready_time(void)
{
    int32_t ret;
    int32_t refresh_time;

    ret = mlx90632_get_meas_type();
    if (ret < 0)
        return ret;

    if ((ret != MLX90632_MTYP_MEDICAL_BURST) && (ret != MLX90632_MTYP_EXTENDED_BURST))
        return -EINVAL;

    if (ret == MLX90632_MTYP_MEDICAL_BURST)
    {
        ret = mlx90632_get_measurement_time(MLX90632_EE_MEDICAL_MEAS1);
        if (ret < 0)
            return ret;

        refresh_time = ret;

        ret = mlx90632_get_measurement_time(MLX90632_EE_MEDICAL_MEAS2);
        if (ret < 0)
            return ret;

        refresh_time = refresh_time + ret;
    }
    else
    {
        ret = mlx90632_get_measurement_time(MLX90632_EE_EXTENDED_MEAS1);
        if (ret < 0)
            return ret;

        refresh_time = ret;

        ret = mlx90632_get_measurement_time(MLX90632_EE_EXTENDED_MEAS2);
        if (ret < 0)
            return ret;

        refresh_time = refresh_time + ret;

        ret = mlx90632_get_measurement_time(MLX90632_EE_EXTENDED_MEAS3);
        if (ret < 0)
            return ret;

        refresh_time = refresh_time + ret;
    }

    return refresh_time;
}

int32_t mlx90632_start_measurement_burst(void)
{
    int32_t ret;
    int tries = MLX90632_MAX_NUMBER_MESUREMENT_READ_TRIES;
    uint16_t reg;

    ret = mlx_read(MLX90632_REG_CTRL, &reg);
    if (ret < 0)
        return ret;

    reg |= MLX90632_START_BURST_MEAS;

    ret = mlx_write(MLX90632_REG_CTRL, reg);
    if (ret < 0)
        return ret;

    ret = mlx90632_calculate_dataset_ready_time();
    if (ret < 0)
        return ret;
    msleep(ret); /* Waiting for refresh of all the measurement tables */

    while (tries-- > 0)
    {
        ret = mlx_read(MLX90632_REG_STATUS, &reg);
        if (ret < 0)
            return ret;
        if ((reg & MLX90632_STAT_BUSY) == 0)
            break;
        /* minimum wait time to complete measurement
         * should be calculated according to refresh rate
         * atm 10ms - 11ms
         */
        usleep(10000, 11000);
    }

    if (tries < 0)
    {
        // data not ready
        return -ETIMEDOUT;
    }

    return 0;
}


static int32_t mlx90632_unlock_eeporm()
{
    return mlx_write(0x3005, MLX90632_EEPROM_WRITE_KEY);
}

static int32_t mlx90632_wait_for_eeprom_not_busy()
{
    uint16_t reg_status;
    int32_t ret = mlx_read(MLX90632_REG_STATUS, &reg_status);

 while (ret >= 0 && reg_status & MLX90632_STAT_EE_BUSY)
 {
     ret = mlx_read(MLX90632_REG_STATUS, &reg_status);
 }

 return ret;
}

static int32_t mlx90632_erase_eeprom(uint16_t address)
{
    int32_t ret = mlx90632_unlock_eeporm();

    if (ret < 0)
        return ret;

    ret = mlx_write(address, 0x00);
    if (ret < 0)
        return ret;

    ret = mlx90632_wait_for_eeprom_not_busy();
    return ret;
}

static int32_t mlx90632_write_eeprom(uint16_t address, uint16_t data)
{
    int32_t ret = mlx90632_erase_eeprom(address);

    if (ret < 0)
        return ret;

    ret = mlx90632_unlock_eeporm();
    if (ret < 0)
        return ret;

    ret = mlx_write(address, data);
    if (ret < 0)
        return ret;

    ret = mlx90632_wait_for_eeprom_not_busy();
    return ret;
}

static int32_t mlx90632_set_refresh_rate(mlx90632_meas_t measRate)
{
    uint16_t meas1, meas2;

    int32_t ret = mlx_read(MLX90632_EE_MEDICAL_MEAS1, &meas1);

    if (ret < 0)
        return ret;

    uint16_t new_value = MLX90632_NEW_REG_VALUE(meas1, measRate, MLX90632_EE_REFRESH_RATE_START, MLX90632_EE_REFRESH_RATE_SHIFT);

    if (meas1 != new_value)
    {
        ret = mlx90632_write_eeprom(MLX90632_EE_MEDICAL_MEAS1, new_value);
        if (ret < 0)
            return ret;
    }

    ret = mlx_read(MLX90632_EE_MEDICAL_MEAS2, &meas2);
    if (ret < 0)
        return ret;

    new_value = MLX90632_NEW_REG_VALUE(meas2, measRate, MLX90632_EE_REFRESH_RATE_START, MLX90632_EE_REFRESH_RATE_SHIFT);
    if (meas2 != new_value)
    {
        ret = mlx90632_write_eeprom(MLX90632_EE_MEDICAL_MEAS2, new_value);
    }

    return ret;
}

static mlx90632_meas_t mlx90632_get_refresh_rate(void)
{
    int32_t ret;
    uint16_t meas1;

    ret = mlx_read(MLX90632_EE_MEDICAL_MEAS1, &meas1);
    if (ret < 0)
        return MLX90632_MEAS_HZ_ERROR;

    return (mlx90632_meas_t)log2(MLX90632_REFRESH_RATE(meas1));
}

/////////////////////////////////////////////////////////////////////////

int mlx90632_init(void)
{
    int r;
    uint16_t eeprom_version=0, reg_status=0;

    mlxHandle = i2c2Handle;
    
    r = mlx_read(MLX90632_EE_VERSION, &eeprom_version);
    if (r < 0) {
        return r;
    }

    if ((eeprom_version & 0x00FF) != MLX90632_DSPv5)
    {
        // this here can fail because of big/little endian of cpu/i2c
        return -EPROTONOSUPPORT;
    }

    r = mlx_read(MLX90632_REG_STATUS, &reg_status);
    if (r < 0)
        return r;

    // Prepare a clean start with setting NEW_DATA to 0
    r = mlx_write(MLX90632_REG_STATUS, reg_status & ~(MLX90632_STAT_DATA_RDY));
    if (r < 0)
        return r;

    if ((eeprom_version & 0x7F00) == MLX90632_XTD_RNG_KEY)
    {
        return ERANGE;
    }

    //mlx90632_test();
    
    return 0;
}



void mlx90632_test(void)
{
    static U16 reg=0x2405, tmp=0x38;
    static F32 temp;
    
    while(1) {
        //mlx_read(reg, &tmp);
        mlx90632_get(&temp);
        delay_ms(50);
    }
    
}


int mlx90632_get(F32 *temp)
{
    int32_t ret = 0; 
    double pre_ambient,pre_object,ambient,object; 
    int16_t Ha,Hb,Gb,Ka;
    int32_t PR,PG,PT,PO,Ea,Eb,Fa,Fb,Ga;
    int16_t ambient_new_raw,ambient_old_raw,object_new_raw,object_old_raw;
    
    /* Read sensor EEPROM registers needed for calcualtions */
    mlx_e2p_read(&PR, &PG, &PO, &PT, &Ea, &Eb, &Fa, &Fb, &Ga, &Gb, &Ha, &Hb, &Ka);
    
    /* Now we read current ambient and object temperature */
    ret = mlx90632_read_temp_raw(&ambient_new_raw, &ambient_old_raw,
                                 &object_new_raw, &object_old_raw);
    if(ret < 0)
        return ret;
    
    /* Get preprocessed temperatures needed for object temperature calculation */
    pre_ambient = mlx90632_preprocess_temp_ambient(ambient_new_raw, ambient_old_raw, Gb);
    pre_object = mlx90632_preprocess_temp_object(object_new_raw, object_old_raw,
                                                 ambient_new_raw, ambient_old_raw, Ka);
    
    mlx90632_set_emissivity(1.0);
    ambient = mlx90632_calc_temp_ambient(ambient_new_raw, ambient_old_raw, PT, PR, PG, PO, Gb);
    object  = mlx90632_calc_temp_object(pre_object, pre_ambient, Ea, Eb, Ga, Fa, Fb, Ha, Hb);
    if(temp) *temp = object;
    
    return 0;
}




