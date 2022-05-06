#include <stdio.h>
#include <string.h>
#include "lcd2.h"
#include "yaoxy.h"
#include "myCfg.h"


#define SWAP16(a)   ((U16)((a<<8)|(a>>8)))

#define LCD_BUF_SIZE (1000*4)
static U8 lcd_buf[LCD_BUF_SIZE];

static lcd_cfg_t lcdCfg={0};


static void lcd_write_dat(U8 *data, U32 len)
{
    //yaoxy_write(ST7789_DAT, data, len);
}
static void lcd_set_rect(U16 x1, U16 y1, U16 x2, U16 y2)
{
    //st7789_set_rect(x1, y1, x2, y2);
}


static inline void lcd_draw_pixel(U16 x, U16 y, U16 color)
{
    U16 c=SWAP16(color);
    
    lcd_set_rect(x, y, x, y);
    lcd_write_dat((U8*)&c, 2);
}


////////////////////////////////////////////////////////////
void lcd_init(lcd_cfg_t *cfg)
{
    if(!cfg) {
        return;
    }
    lcdCfg = *cfg;

    yaoxy_init();
}


void lcd_rotate(U8 angle)
{
    
}


void lcd_set_backlight(U8 on)
{
    
}


void lcd_fill(U16 color)
{   
    lcd_fill_rect(0, 0, lcdCfg.width-1, lcdCfg.height-1, color);
}


void lcd_draw_line(U16 x1, U16 y1, U16 x2, U16 y2, U16 color)
{
    
}



void lcd_draw_rect(U16 x1, U16 y1, U16 w, U16 h, U16 color)
{
    U16  x2,y2;

    x2 = x1 + w;
    y2 = y1 + h;
    lcd_draw_line(x1,y1,x2,y1,color);
    lcd_draw_line(x1,y1,x1,y2,color);
    lcd_draw_line(x1,y2,x2,y2,color);
    lcd_draw_line(x2,y1,x2,y2,color);
}


void lcd_fill_rect(U16 x1, U16 y1, U16 w, U16 h, U16 color)
{
    
}



void lcd_draw_char(U16 x, U16 y, U8 c, U8 font, U16 color, U16 bgcolor)
{
    
} 



void lcd_draw_string(U16 x, U16 y, U16 w, U16 h, U8 *str, U8 font, U16 color, U16 bgcolor, U8 hori, U8 vert)
{
    yaoxy_write(x, y, str);
}


