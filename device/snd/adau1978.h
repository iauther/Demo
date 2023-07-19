#ifndef __ADAU1978_Hx__
#define __ADAU1978_Hx__

#include "types.h"

int adau1978_init(void);
int adau1978_read_byte(U16 addr, U8 *data);
int adau1978_write_byte(U16 addr, U8 *data);
int adau1978_read(U16 addr, U8 *data, U16 len);
int adau1978_write(U16 addr, U8 *data, U16 len);
int adau1978_test(void);
#endif



