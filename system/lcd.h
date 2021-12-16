#ifndef __LCD_H__
#define __LCD_H__

#include "types.h"
#include "myCfg.h"

/* 颜色 */
#if (LCD_COLOR==COLOR_RGB565)
    #define COLOR_WHITE         	 0xFFFF
    #define COLOR_BLACK         	 0x0000	  
    #define COLOR_BLUE               0x001F  
    #define COLOR_BRED               0XF81F
    #define COLOR_GRED               0XFFE0
    #define COLOR_GBLUE              0X07FF
    #define COLOR_RED           	 0xF800
    #define COLOR_MAGENTA       	 0xF81F
    #define COLOR_GREEN         	 0x07E0
    #define COLOR_CYAN          	 0x7FFF
    #define COLOR_YELLOW        	 0xFFE0
    #define COLOR_BROWN 			 0XBC40
    #define COLOR_BRRED 			 0XFC07
    #define COLOR_GRAY  			 0X8430
#elif (LCD_COLOR==COLOR_RGB666)
    
#elif (LCD_COLOR==COLOR_RGB888)

#else


#endif

enum {
    ANGLE_0=0,
    ANGLE_90,
    ANGLE_180,
    ANGLE_270,
};

enum {
    HORIZONTAL_LEFT=0,
    HORIZONTAL_CENTER,
    HORIZONTAL_RIGHT,
};

enum {
    VERTICAL_TOP=0,
    VERTICAL_CENTER,
    VERTICAL_BOTTOM,
};

enum {
    COLOR_RGB8=0,
    COLOR_RGB565,
    COLOR_RGB666,
    COLOR_RGB888,
    COLOR_RGB32,
};

typedef struct {
    U8   color;
    U16  width;
    U16  height;
}lcd_cfg_t;


void lcd_init(lcd_cfg_t *cfg);
void lcd_rotate(U8 angle);
void lcd_set_backlight(U8 on);
void lcd_clear(U16 color);
void lcd_draw_line(U16 x1, U16 y1, U16 x2, U16 y2, U16 color);
void lcd_draw_rect(U16 x1, U16 y1, U16 w, U16 h, uint16_t color);
void lcd_fill_rect(U16 x1, U16 y1, U16 w, U16 h, U16 color);

void lcd_draw_char(U16 x, U16 y, U8 c, U8 font, U16 color, U16 bgcolor);
void lcd_draw_string(U16 x, U16 y, U16 width, U16 height, U8 *str, U8 font, U16 color, U16 bgcolor, U8 hori, U8 vert);

#endif




