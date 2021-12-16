#ifndef __E2P_Hx__
#define __E2P_Hx__

#include "types.h"

int e2p_init(void);

int e2p_read_byte(U32 addr, U8* data);

int e2p_write_byte(U32 addr, U8* data);

int e2p_read(U32 addr, U8 *data, U16 len);

int e2p_write(U32 addr, U8 *data, U16 len);

void e2p_test(void);

#endif

