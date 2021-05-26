#ifndef _MLX90632_DEPENDS_LIB_
#define _MLX90632_DEPENDS_LIB_


extern int32_t mlx90632_i2c_read(int16_t register_address, uint16_t *value);

extern int32_t mlx90632_i2c_write(int16_t register_address, uint16_t value);

extern void usleep(int min_range, int max_range);

extern void msleep(int msecs);

#endif
