#ifndef _MLX90632_LIB_
#define _MLX90632_LIB_

/* Including CRC calculation functions */
#include "types.h"

/* Solve errno not defined values */
#ifndef ETIMEDOUT
#define ETIMEDOUT 110 
#endif
#ifndef EINVAL
#define EINVAL 22 
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT 93 
#endif
#ifndef ERANGE
#define ERANGE 34 
#endif
#ifndef ENOKEY
#define ENOKEY 126 
#endif


/* BIT, GENMASK and ARRAY_SIZE macros are imported from kernel */
#ifndef BIT
#define BIT(x)(1UL << (x))
#endif
#ifndef GENMASK
#ifndef BITS_PER_LONG
//#warning "Using default BITS_PER_LONG value"
#define BITS_PER_LONG 32 
#endif
#define GENMASK(h, l) \
    (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#endif
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0])) 
#endif

/* Memory sections addresses */
#define MLX90632_ADDR_RAM   0x4000 
#define MLX90632_ADDR_EEPROM    0x2480 
/* EEPROM addresses - used at startup */
#define MLX90632_EE_CTRL    0x24d4 
#define MLX90632_EE_CONTROL MLX90632_EE_CTRL 
#define MLX90632_EE_I2C_ADDRESS 0x24d5 
#define MLX90632_EE_VERSION 0x240b 
#define MLX90632_EE_P_R     0x240c 
#define MLX90632_EE_P_G     0x240e 
#define MLX90632_EE_P_T     0x2410 
#define MLX90632_EE_P_O     0x2412 
#define MLX90632_EE_Aa      0x2414 
#define MLX90632_EE_Ab      0x2416 
#define MLX90632_EE_Ba      0x2418 
#define MLX90632_EE_Bb      0x241a 
#define MLX90632_EE_Ca      0x241c 
#define MLX90632_EE_Cb      0x241e 
#define MLX90632_EE_Da      0x2420 
#define MLX90632_EE_Db      0x2422 
#define MLX90632_EE_Ea      0x2424 
#define MLX90632_EE_Eb      0x2426 
#define MLX90632_EE_Fa      0x2428 
#define MLX90632_EE_Fb      0x242a 
#define MLX90632_EE_Ga      0x242c 
#define MLX90632_EE_Gb      0x242e 
#define MLX90632_EE_Ka      0x242f 
#define MLX90632_EE_Ha      0x2481 
#define MLX90632_EE_Hb      0x2482 
#define MLX90632_EE_MEDICAL_MEAS1      0x24E1 
#define MLX90632_EE_MEDICAL_MEAS2      0x24E2 
#define MLX90632_EE_EXTENDED_MEAS1     0x24F1 
#define MLX90632_EE_EXTENDED_MEAS2     0x24F2 
#define MLX90632_EE_EXTENDED_MEAS3     0x24F3 
/* Refresh Rate */
typedef enum mlx90632_meas_e {
    MLX90632_MEAS_HZ_ERROR = -1,
    MLX90632_MEAS_HZ_HALF = 0,
    MLX90632_MEAS_HZ_1 = 1,
    MLX90632_MEAS_HZ_2 = 2,
    MLX90632_MEAS_HZ_4 = 3,
    MLX90632_MEAS_HZ_8 = 4,
    MLX90632_MEAS_HZ_16 = 5,
    MLX90632_MEAS_HZ_32 = 6,
    MLX90632_MEAS_HZ_64 = 7,
} mlx90632_meas_t;
#define MLX90632_EE_REFRESH_RATE_START 10 
#define MLX90632_EE_REFRESH_RATE_SHIFT 8 
#define MLX90632_EE_REFRESH_RATE_MASK GENMASK(MLX90632_EE_REFRESH_RATE_START, MLX90632_EE_REFRESH_RATE_SHIFT) 
#define MLX90632_EE_REFRESH_RATE(ee_val) (ee_val & MLX90632_EE_REFRESH_RATE_MASK) 
#define MLX90632_REFRESH_RATE(ee_val) (MLX90632_EE_REFRESH_RATE(ee_val) >> MLX90632_EE_REFRESH_RATE_SHIFT) 
#define MLX90632_REFRESH_RATE_STATUS(mlx90632_meas) (mlx90632_meas << MLX90632_EE_REFRESH_RATE_SHIFT)  
/* Register addresses - volatile */
#define MLX90632_REG_I2C_ADDR   0x3000 
/* Control register address - volatile */
#define MLX90632_REG_CTRL   0x3001 
#define MLX90632_CFG_SOC_SHIFT 3 
#define MLX90632_CFG_SOC_MASK BIT(MLX90632_CFG_SOC_SHIFT)
#define MLX90632_CFG_PWR_MASK GENMASK(2, 1) 
#define MLX90632_CFG_PWR(ctrl_val) (ctrl_val & MLX90632_CFG_PWR_MASK) 
#define MLX90632_CFG_MTYP_SHIFT 4 
#define MLX90632_CFG_MTYP_MASK GENMASK(8, MLX90632_CFG_MTYP_SHIFT) 
#define MLX90632_CFG_MTYP(ctrl_val) (ctrl_val & MLX90632_CFG_MTYP_MASK) 
#define MLX90632_CFG_SOB_SHIFT 11 
#define MLX90632_CFG_SOB_MASK BIT(MLX90632_CFG_SOB_SHIFT)
#define MLX90632_CFG_SOB(ctrl_val) (ctrl_val << MLX90632_CFG_SOB_SHIFT)

/* PowerModes statuses */
#define MLX90632_PWR_STATUS(ctrl_val) (ctrl_val << 1)
#define MLX90632_PWR_STATUS_HALT MLX90632_PWR_STATUS(0) 
#define MLX90632_PWR_STATUS_SLEEP_STEP MLX90632_PWR_STATUS(1) 
#define MLX90632_PWR_STATUS_STEP MLX90632_PWR_STATUS(2) 
#define MLX90632_PWR_STATUS_CONTINUOUS MLX90632_PWR_STATUS(3) 
/* Measurement type select*/
#define MLX90632_MTYP_STATUS(ctrl_val) (ctrl_val << MLX90632_CFG_MTYP_SHIFT)
#define MLX90632_MTYP_STATUS_MEDICAL MLX90632_MTYP_STATUS(0) 
#define MLX90632_MTYP_STATUS_EXTENDED MLX90632_MTYP_STATUS(17) 
#define MLX90632_MTYP(reg_val) (MLX90632_CFG_MTYP(reg_val) >> MLX90632_CFG_MTYP_SHIFT) 
/* Start of burst measurement options */
#define MLX90632_START_BURST_MEAS MLX90632_CFG_SOB(1)
#define MLX90632_BURST_MEAS_NOT_PENDING MLX90632_CFG_SOB(0)

/* Device status register - volatile */
#define MLX90632_REG_STATUS 0x3fff 
#define MLX90632_STAT_BUSY    BIT(10) 
#define MLX90632_STAT_EE_BUSY BIT(9) 
#define MLX90632_STAT_BRST    BIT(8) 
#define MLX90632_STAT_CYCLE_POS GENMASK(6, 2) 
#define MLX90632_STAT_DATA_RDY    BIT(0) 
/* RAM_MEAS address-es for each channel */
#define MLX90632_RAM_1(meas_num)    (MLX90632_ADDR_RAM + 3 * meas_num)
#define MLX90632_RAM_2(meas_num)    (MLX90632_ADDR_RAM + 3 * meas_num + 1)
#define MLX90632_RAM_3(meas_num)    (MLX90632_ADDR_RAM + 3 * meas_num + 2)

/* Timings (ms) */
#define MLX90632_TIMING_EEPROM 100 
/* Magic constants */
#define MLX90632_DSPv5 0x05 /* EEPROM DSP version */
#define MLX90632_EEPROM_VERSION MLX90632_ID_MEDICAL 
#define MLX90632_EEPROM_WRITE_KEY 0x554C 
#define MLX90632_RESET_CMD  0x0006 
#define MLX90632_MAX_MEAS_NUM   31 
#define MLX90632_EE_SEED    0x3f6d 
#define MLX90632_REF_12 12.0 
#define MLX90632_REF_3  12.0 
#define MLX90632_XTD_RNG_KEY 0x0500 
/* Measurement types - the MSBit is for software purposes only and has no hardware bit related to it. It indicates continuous '0' or sleeping step burst - '1' measurement mode*/
#define MLX90632_MTYP_MEDICAL 0x00
#define MLX90632_MTYP_EXTENDED 0x11
#define MLX90632_MTYP_MEDICAL_BURST 0x80
#define MLX90632_MTYP_EXTENDED_BURST 0x91
#define MLX90632_BURST_MEASUREMENT_TYPE(meas_type) (meas_type + 0x80) 
#define MLX90632_MEASUREMENT_TYPE_STATUS(mtyp_type) (mtyp_type & 0x7F) 
#define MLX90632_MEASUREMENT_BURST_STATUS(mtyp_type) (mtyp_type & 0x80) 
#define MLX90632_MEAS_MAX_TIME 2000 
#define MLX90632_MAX_NUMBER_MESUREMENT_READ_TRIES 100 
/* Gets a new register value based on the old register value - only writing the value based on the desired bits
 * Masks the old register and shifts the new value in
 */
#define MLX90632_NEW_REG_VALUE(old_reg, new_value, h, l) \
    ((old_reg & (0xFFFF ^ GENMASK(h, l))) | (new_value << MLX90632_EE_REFRESH_RATE_SHIFT))


int32_t mlx90632_addressed_reset(void);
double mlx90632_get_emissivity(void);
int mlx90632_start_measurement(void);
int32_t mlx90632_start_measurement_burst(void);


//////////////////////////////////////////////////////////////////////////////////////////////

int mlx90632_init(handle_t h);
void mlx90632_test(void);
int mlx90632_get(F32 *temp);


#endif

