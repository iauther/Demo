#ifndef __ST7789_Hx__
#define __ST7789_Hx__

#include "types.h"

enum {
    ST7789_CMD=0,
    ST7789_DAT,
    ST7789_DLY,
    
    ST7789_MAX
};

int st7789_init(void);

int st7789_reset(void);
    
int st7789_write(U8 mode, U8 *data, U32 len);

int st7789_set_rect(U16 x1, U16 y1, U16 x2, U16 y2);

void st7789_set_blk(U8 on);

#endif

