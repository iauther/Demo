#include <stdio.h>
#include <string.h>
#include "lcd.h"
#include "font.h"
#include "st7789.h"



#define SWAP16(a)   ((U16)((a<<8)|(a>>8)))


#define LCD_BUF_SIZE (1000*4)
static U8 lcd_buf[LCD_BUF_SIZE];
static lcd_cfg_t lcdCfg={0};


static void lcd_write_dat(U8 *data, U32 len)
{
    st7789_write(ST7789_DAT, data, len);
}
static void lcd_set_rect(U16 x1, U16 y1, U16 x2, U16 y2)
{
    st7789_set_rect(x1, y1, x2, y2);
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
    
    st7789_init();
}


void lcd_rotate(U8 angle)
{
    
}


void lcd_set_backlight(U8 on)
{
    st7789_set_blk(on);
}


void lcd_fill(U16 color)
{   
    lcd_fill_rect(0, 0, lcdCfg.width-1, lcdCfg.height-1, color);
}


void lcd_draw_line(U16 x1, U16 y1, U16 x2, U16 y2, U16 color)
{
    U16  delta_x = 0, delta_y = 0;
    S8   incx = 0, incy = 0;
    U16  distance = 0;
    U16  t = 0;
    U16  x = 0, y = 0;
    U16  x_temp = 0, y_temp = 0;

    /* 画斜线（Bresenham算法） */
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    if(delta_x > 0) {
        //斜线(从左到右)
        incx = 1;
    }
    else if(delta_x == 0) {
        //垂直斜线(竖线)
        incx = 0;
    }
    else {
        //斜线(从右到左)
        incx = -1;
        delta_x = -delta_x;
    }
    
    if(delta_y > 0) {
        //斜线(从左到右)
        incy = 1;
    }
    else if(delta_y == 0) {
        //水平斜线(水平线)
        incy = 0;
    }
    else {
        //斜线(从右到左)
        incy = -1;
        delta_y = -delta_y;
    }           
    
    /* 计算画笔打点距离(取两个间距中的最大值) */
    if(delta_x > delta_y) {
        distance = delta_x;
    }
    else {
        distance = delta_y;
    }
    
    /* 开始打点 */
    x = x1;
    y = y1;
    //第一个点无效，所以t的次数加一
    for(t = 0; t <= distance + 1;t++) {
        lcd_draw_pixel(x, y, color);
    
        /* 判断离实际值最近的像素点 */
        x_temp += delta_x;  
        if(x_temp > distance) {
            //x方向越界，减去距离值，为下一次检测做准备
            x_temp -= distance;     
            //在x方向递增打点
            x += incx;
                
        }
        y_temp += delta_y;
        if(y_temp > distance) {
            //y方向越界，减去距离值，为下一次检测做准备
            y_temp -= distance;
            //在y方向递增打点
            y += incy;
        }
    }
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
    U16 i,j;
    U16 x2,y2;
    U16 c=SWAP16(color);
    U32 size = 0, size_remain = 0; 
    
    size = (w + 1) * (h + 1) * 2;
    if(size > LCD_BUF_SIZE) {
        size_remain = size - LCD_BUF_SIZE;
        size = LCD_BUF_SIZE;
    }
    
    lcd_set_rect(x1, y1, x2, y2);

    while(1) {
        for(i = 0; i < size / 2; i++) {
            lcd_buf[2 * i] = color >> 8;
            lcd_buf[2 * i + 1] = color & 0xff;
        }

        lcd_write_dat(lcd_buf, size);

        if(size_remain == 0)
            break;

        if(size_remain > LCD_BUF_SIZE) {
            size_remain = size_remain - LCD_BUF_SIZE;
        }
        else {
            size = size_remain;
            size_remain = 0;
        }
    }
}


static U16 chrBuf[2000];
void lcd_draw_char(U16 x, U16 y, U8 c, U8 font, U16 color, U16 bgcolor)
{
    U8 tmp;
    U16 i, j;
    U16 x0=x,y0=y;
    const U8* fontDat;
    font_info_t info = font_get_info(font);
    
    fontDat = font_get_data(c, font);
    for(i = 0; i < info.size; i++)  {/*遍历打印所有像素点到LCD */
        tmp = fontDat[i];
        for(j = 0; j < 8; j++)  { /* 打印一个像素点到液晶 */
                       
            if(tmp & 0x80) {
                chrBuf[(y-y0)*info.width+x-x0] = SWAP16(color);
            }
            else {
                chrBuf[(y-y0)*info.width+x-x0] = SWAP16(bgcolor);
            }
            tmp <<= 1;
            y++;
            
            if(y >= lcdCfg.height) {
                return;     /* 超区域了 */
            }
            
            if((y - y0) == info.height) {
                y = y0;
                x++;
                if(x >= lcdCfg.width) {
                    return; /* 超区域了 */
                }
                break;
            }
        }    
    }
    
    lcd_set_rect(x, y, x+info.width-1, y+info.height-1);
    lcd_write_dat((U8*)chrBuf, info.width*info.height*2);
} 


static void draw_string(U16 x, U16 y, U16 w, U16 h, U8 *str, U8 font, U16 color, U16 bgcolor)
{
    U16 x0=x;
    U16 xmax,ymax;
    font_info_t info = font_get_info(font);
    
    xmax = x+w;
    ymax = y+h;
    
    while((*str<='~') && (*str>=' '))  {      /* 判断是不是非法字符! */
          
        if(x >= xmax || y >= ymax) {
            break;
        }
        
        lcd_draw_char(x, y, *str, font, color, bgcolor);
        x += info.width;
        str++;
    }  
}



void lcd_draw_string(U16 x, U16 y, U16 w, U16 h, U8 *str, U8 font, U16 color, U16 bgcolor, U8 hori, U8 vert)
{
    U16  sw;
    rect_t rect;
    font_info_t info=font_get_info(font);
    
    if(info.height>h) {
        return;
    }
    
    sw = strlen((char*)str)*info.width;
    if(sw>w) {
        sw = w;
    }
    
    if(hori==HORIZONTAL_LEFT) {
        rect.x = x;
    }
    else if(hori==HORIZONTAL_RIGHT) {
        rect.x = x+w-sw;
    }
    else if(hori==HORIZONTAL_CENTER) {
        rect.x = x+(w-sw)/2;
    }
    else {
        return;
    }
    
    if(vert==VERTICAL_TOP) {
        rect.y = y;
    }
    else if(vert==VERTICAL_BOTTOM) {
        rect.y = y+h-info.height;
    }
    else if(vert==VERTICAL_CENTER) {
        rect.y = y+(h-info.height)/2;
    }
    else {
        return;
    }
    
    rect.h = info.height;
    
    
    if(rect.x+sw>=lcdCfg.width) {
        rect.w = lcdCfg.width-rect.x;
    }
    else {
        rect.w = sw;
    }
    
    draw_string(rect.x, rect.y, rect.w, rect.h, str, font, color, bgcolor);
}


