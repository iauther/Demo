#ifndef __PCM1864_Hx__
#define __PCM1864_Hx__

#include "types.h"

int pcm1864_init(void);
int pcm1864_read_byte(U16 addr, U8 *data);
int pcm1864_write_byte(U16 addr, U8 *data);
int pcm1864_read(U16 addr, U8 *data, U16 len);
int pcm1864_write(U16 addr, U8 *data, U16 len);
int pcm1864_test(void);
#endif



