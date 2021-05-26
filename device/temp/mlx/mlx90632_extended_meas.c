#include <stdint.h>
#include <math.h>
#include <errno.h>

#include "mlx90632.h"
#include "mlx90632_depends.h"

#define POW10 10000000000LL

#ifndef VERSION
#define VERSION "test"
#endif

static int32_t mlx90632_read_temp_ambient_raw_extended(int16_t *ambient_new_raw, int16_t *ambient_old_raw)
{
    int32_t ret;
    uint16_t read_tmp;

    ret = mlx90632_i2c_read(MLX90632_RAM_3(17), &read_tmp);
    if (ret < 0)
        return ret;
    *ambient_new_raw = (int16_t)read_tmp;

    ret = mlx90632_i2c_read(MLX90632_RAM_3(18), &read_tmp);
    if (ret < 0)
        return ret;
    *ambient_old_raw = (int16_t)read_tmp;

    return ret;
}

static int32_t mlx90632_read_temp_object_raw_extended(int16_t *object_new_raw)
{
    int32_t ret;
    uint16_t read_tmp;
    int32_t read;

    ret = mlx90632_i2c_read(MLX90632_RAM_1(17), &read_tmp);
    if (ret < 0)
        return ret;

    read = (int16_t)read_tmp;

    ret = mlx90632_i2c_read(MLX90632_RAM_2(17), &read_tmp);
    if (ret < 0)
        return ret;

    read = read - (int16_t)read_tmp;

    ret = mlx90632_i2c_read(MLX90632_RAM_1(18), &read_tmp);
    if (ret < 0)
        return ret;

    read = read - (int16_t)read_tmp;

    ret = mlx90632_i2c_read(MLX90632_RAM_2(18), &read_tmp);
    if (ret < 0)
        return ret;

    read = (read + (int16_t)read_tmp) / 2;

    ret = mlx90632_i2c_read(MLX90632_RAM_1(19), &read_tmp);
    if (ret < 0)
        return ret;

    read = read + (int16_t)read_tmp;

    ret = mlx90632_i2c_read(MLX90632_RAM_2(19), &read_tmp);
    if (ret < 0)
        return ret;

    read = read + (int16_t)read_tmp;

    if (read > 32767 || read < -32768)
        return -EINVAL;

    *object_new_raw = (int16_t)read;

    return ret;
}

int32_t mlx90632_read_temp_raw_extended(int16_t *ambient_new_raw, int16_t *ambient_old_raw, int16_t *object_new_raw)
{
    int32_t ret, start_measurement_ret;
    int tries = 3;

    // trigger and wait for measurement to complete
    while (tries-- > 0)
    {
        start_measurement_ret = mlx90632_start_measurement();
        if (start_measurement_ret < 0)
            return start_measurement_ret;

        if (start_measurement_ret == 19)
            break;
    }

    if (tries < 0)
    {
        // data not ready
        return -ETIMEDOUT;
    }

    ret = mlx90632_read_temp_ambient_raw_extended(ambient_new_raw, ambient_old_raw);
    if (ret < 0)
        return ret;

    ret = mlx90632_read_temp_object_raw_extended(object_new_raw);

    return ret;
}

int32_t mlx90632_read_temp_raw_extended_burst(int16_t *ambient_new_raw, int16_t *ambient_old_raw, int16_t *object_new_raw)
{
    int32_t ret, start_measurement_ret;

    // trigger and wait for measurement to complete
    start_measurement_ret = mlx90632_start_measurement_burst();
    if (start_measurement_ret < 0)
        return start_measurement_ret;

    ret = mlx90632_read_temp_ambient_raw_extended(ambient_new_raw, ambient_old_raw);
    if (ret < 0)
        return ret;

    ret = mlx90632_read_temp_object_raw_extended(object_new_raw);

    return ret;
}

double mlx90632_preprocess_temp_ambient_extended(int16_t ambient_new_raw, int16_t ambient_old_raw, int16_t Gb)
{
    double VR_Ta, kGb;

    kGb = ((double)Gb) / 1024.0;

    VR_Ta = ambient_old_raw + kGb * (ambient_new_raw / (MLX90632_REF_3));
    return ((ambient_new_raw / (MLX90632_REF_3)) / VR_Ta) * 524288.0;
}

double mlx90632_preprocess_temp_object_extended(int16_t object_new_raw, int16_t ambient_new_raw,
                                                int16_t ambient_old_raw, int16_t Ka)
{
    double VR_IR, kKa;

    kKa = ((double)Ka) / 1024.0;

    VR_IR = ambient_old_raw + kKa * (ambient_new_raw / (MLX90632_REF_3));
    return ((object_new_raw / (MLX90632_REF_12)) / VR_IR) * 524288.0;
}

double mlx90632_calc_temp_ambient_extended(int16_t ambient_new_raw, int16_t ambient_old_raw, int32_t P_T,
                                           int32_t P_R, int32_t P_G, int32_t P_O, int16_t Gb)
{
    double Asub, Bsub, Ablock, Bblock, Cblock, AMB;

    AMB = mlx90632_preprocess_temp_ambient_extended(ambient_new_raw, ambient_old_raw, Gb);

    Asub = ((double)P_T) / (double)17592186044416.0;
    Bsub = AMB - ((double)P_R / (double)256.0);
    Ablock = Asub * (Bsub * Bsub);
    Bblock = (Bsub / (double)P_G) * (double)1048576.0;
    Cblock = (double)P_O / (double)256.0;

    return Bblock + Ablock + Cblock;
}

static double mlx90632_calc_temp_object_iteration_extended(double prev_object_temp, int32_t object, double TAdut, double TaTr4,
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
    Alpha_corr = (((double)(Fa * POW10)) * Ha_customer * (double)(1.0 + calcedGa + calcedGb)) /
                 ((double)70368744177664.0);
    calcedFa = object / (emissivity * (Alpha_corr / POW10));

    first_sqrt = sqrt(calcedFa + TaTr4);

    return sqrt(first_sqrt) - 273.15 - Hb_customer;
}

double mlx90632_calc_temp_object_extended(int32_t object, int32_t ambient, double reflected,
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
        temp = mlx90632_calc_temp_object_iteration_extended(temp, object, TAdut, TaTr4, Ga, Fa / 2, Fb, Ha, Hb, tmp_emi);
    }

    return temp;
}

int32_t mlx90632_set_meas_type(uint8_t type)
{
    int32_t ret;
    uint16_t reg_ctrl;

    if ((type != MLX90632_MTYP_MEDICAL) & (type != MLX90632_MTYP_EXTENDED) & (type != MLX90632_MTYP_MEDICAL_BURST) & (type != MLX90632_MTYP_EXTENDED_BURST))
        return -EINVAL;

    ret = mlx90632_addressed_reset();
    if (ret < 0)
        return ret;

    ret = mlx90632_i2c_read(MLX90632_REG_CTRL, &reg_ctrl);
    if (ret < 0)
        return ret;

    reg_ctrl = reg_ctrl & (~MLX90632_CFG_MTYP_MASK & ~MLX90632_CFG_PWR_MASK);
    reg_ctrl |= (MLX90632_MTYP_STATUS(MLX90632_MEASUREMENT_TYPE_STATUS(type)) | MLX90632_PWR_STATUS_HALT);

    ret = mlx90632_i2c_write(MLX90632_REG_CTRL, reg_ctrl);
    if (ret < 0)
        return ret;

    ret = mlx90632_i2c_read(MLX90632_REG_CTRL, &reg_ctrl);
    if (ret < 0)
        return ret;

    reg_ctrl = reg_ctrl & ~MLX90632_CFG_PWR_MASK;
    if (MLX90632_MEASUREMENT_BURST_STATUS(type))
    {
        reg_ctrl |= MLX90632_PWR_STATUS_SLEEP_STEP;
    }
    else
    {
        reg_ctrl |= MLX90632_PWR_STATUS_CONTINUOUS;
    }

    ret = mlx90632_i2c_write(MLX90632_REG_CTRL, reg_ctrl);

    return ret;
}

int32_t mlx90632_get_meas_type(void)
{
    int32_t ret;
    uint16_t reg_ctrl;
    uint16_t reg_temp;

    ret = mlx90632_i2c_read(MLX90632_REG_CTRL, &reg_temp);
    if (ret < 0)
        return ret;

    reg_ctrl = MLX90632_MTYP(reg_temp);

    if ((reg_ctrl != MLX90632_MTYP_MEDICAL) & (reg_ctrl != MLX90632_MTYP_EXTENDED))
        return -EINVAL;

    reg_temp = MLX90632_CFG_PWR(reg_temp);

    if (reg_temp == MLX90632_PWR_STATUS_SLEEP_STEP)
        return MLX90632_BURST_MEASUREMENT_TYPE(reg_ctrl);

    if (reg_temp != MLX90632_PWR_STATUS_CONTINUOUS)
        return -EINVAL;

    return reg_ctrl;
}