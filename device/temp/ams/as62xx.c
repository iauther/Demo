#include "as62xx.h"

static int is_16bit_sensor=0;
static const int as62xx__conv_rates[4][2] = {
                                                {4, 0}, 
                                                {1, 0},
					                            {0, 250000}, 
					                            {0, 125000}
					                       };

static const struct regmap_config AS62XX__regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,
	.max_register = AS62XX__MAX_REGISTER,
	.cache_type = REGCACHE_RBTREE,
	.rd_table = &AS62XX__readable_table,
	.wr_table = &AS62XX__writable_table,
	.volatile_table = &AS62XX__volatile_table,
};

//////////////////////////////////////////////////////////
static int as_i2c_read()
{
    
}
static int as_i2c_write()
{
    return 0;
}




/////////////////////////////////////////////////////////



int AS62XX__read_raw(int *val, int *val2, long mask)
{
	unsigned int reg_val;
	int cr, err;
	s32 ret;

	if (channel->type != IIO_TEMP)
		return -EINVAL;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
	case IIO_CHAN_INFO_PROCESSED:
		err = regmap_read(AS62XX__REG_TVAL,
					&reg_val);
		if (err < 0)
			return err;
		ret = data->is_16bit_sensor ? sign_extend32(reg_val, 15) :
			                sign_extend32(reg_val, 15) >> 4;
		if (mask == IIO_CHAN_INFO_PROCESSED)
			*val = data->is_16bit_sensor ? (ret * 78125) / 10000000 :
				                 (ret * 625) / 10000;
		else
			*val = ret;
		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		if (data->is_16bit_sensor) {
			*val = 7;
			*val2 = 812500;
		} else {
			*val = 62;
			*val2 = 500000;
		}

		return IIO_VAL_INT_PLUS_MICRO;
	case IIO_CHAN_INFO_SAMP_FREQ:
		err = regmap_read(AS62XX__REG_CONFIG,
					&reg_val);
		if (err < 0)
			return err;
		cr = (reg_val & AS62XX__CONFIG_CR_MASK)
			>> AS62XX__CONFIG_CR_SHIFT;
		*val = AS62XX__conv_rates[cr][0];
		*val2 = AS62XX__conv_rates[cr][1];
		return IIO_VAL_INT_PLUS_MICRO;
	default:
		break;
	}
	return -EINVAL;
}

static int AS62XX__write_raw(int val, int val2, long mask)
{
	int i,err;
	unsigned int config_val;

	for (i = 0; i < ARRAY_SIZE(AS62XX__conv_rates); i++)
		if ((val == AS62XX__conv_rates[i][0]) &&
			(val2 == AS62XX__conv_rates[i][1])) {
			err = regmap_read(data->regmap,
				AS62XX__REG_CONFIG, &config_val);
			if (err < 0)
				return err;
			config_val &= ~AS62XX__CONFIG_CR_MASK;
			config_val |= i << AS62XX__CONFIG_CR_SHIFT;

			return reg_write(AS62XX__REG_CONFIG,
							config_val);
		}
	return -1;
}

static int AS62XX__read_thresh(int *val, int *val2)
{
	struct AS62XX__data *data = iio_priv(indio_dev);
	int ret;
	U32 reg_val;

	dev_info(&data->client->dev, "read thresh called\n");
	if (dir == IIO_EV_DIR_RISING)
		ret = reg_read(AS62XX__REG_THIGH, &reg_val);
	else
		ret = reg_read( AS62XX__REG_TLOW, &reg_val);

	*val = data->is_16bit_sensor ? sign_extend32(reg_val, 15) :
			         sign_extend32(reg_val, 15) >> 4;

	if (ret)
		return ret;
		
	return 0;
}

int AS62XX__write_thresh(int val, int val2)
{
	struct AS62XX__data *data = iio_priv(indio_dev);
	int ret;
	S16 value = is_16bit_sensor ? (val & 0xFFF0) : val << 4;

	dev_info(&data->client->dev, "write thresh called %d\n", value);
	if (dir == IIO_EV_DIR_RISING)
		ret = re_write(AS62XX__REG_THIGH, value);
	else
		ret = reg_write(AS62XX__REG_TLOW, value);
	return ret;
}



int AS62XX__init(void)
{
	int err;

	err = reg_write_bits(AS62XX__REG_CONFIG, AS62XX__CONFIG_IM_MASK,
			             AS62XX__CONFIG_INIT_IM << AS62XX__CONFIG_IM_SHIFT);
	if (err < 0)
		return err;
	err = reg_write_bits(AS62XX__REG_CONFIG, AS62XX__CONFIG_POL_MASK,
			             AS62XX__CONFIG_INIT_POL << AS62XX__CONFIG_POL_SHIFT);
	if (err < 0)
		return err;
	err = reg_write_bits(AS62XX__REG_CONFIG, AS62XX__CONFIG_CF_MASK,
			             AS62XX__CONFIG_INIT_CF << AS62XX__CONFIG_CF_SHIFT);
	if (err < 0)
		return err;

	return err;
}





