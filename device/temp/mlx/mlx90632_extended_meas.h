#ifndef _MLX90632_EXTENDED_MEAS_LIB_
#define _MLX90632_EXTENDED_MEAS_LIB_

#include <stdint.h>

int32_t mlx90632_read_temp_raw_extended(int16_t *ambient_new_raw, int16_t *ambient_old_raw, int16_t *object_new_raw);

int32_t mlx90632_read_temp_raw_extended_burst(int16_t *ambient_new_raw, int16_t *ambient_old_raw, int16_t *object_new_raw);

double mlx90632_preprocess_temp_ambient_extended(int16_t ambient_new_raw, int16_t ambient_old_raw, int16_t Gb);

double mlx90632_preprocess_temp_object_extended(int16_t object_new_raw, int16_t ambient_new_raw,
                                                int16_t ambient_old_raw, int16_t Ka);

double mlx90632_calc_temp_ambient_extended(int16_t ambient_new_raw, int16_t ambient_old_raw, int32_t P_T,
                                           int32_t P_R, int32_t P_G, int32_t P_O, int16_t Gb);

double mlx90632_calc_temp_object_extended(int32_t object, int32_t ambient, double reflected,
                                          int32_t Ea, int32_t Eb, int32_t Ga, int32_t Fa, int32_t Fb,
                                          int16_t Ha, int16_t Hb);

int32_t mlx90632_set_meas_type(uint8_t type);

int32_t mlx90632_get_meas_type(void);

#ifdef TEST
int32_t mlx90632_read_temp_ambient_raw_extended(int16_t *ambient_new_raw, int16_t *ambient_old_raw);
int32_t mlx90632_read_temp_object_raw_extended(int16_t *object_new_raw);

#endif

#endif

