#ifndef __FONT_H__
#define __FONT_H__	 

#include "types.h"

//常用ASCII表
//偏移量32 
//ASCII字符集: !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
//PC2LCD2002取模方式设置：阴码+逐列式+顺向+C51格式
//总共：2个字符集（16*16和24*24），用户可以自行新增其他分辨率的字符集。
//每个字符所占用的字节数为:(size/8+((size%8)?1:0))*(size/2),其中size:是字库生成时的点阵大小(12/16/24...)
//1608 ASCII字符集点阵
extern const unsigned char font_1608[95][16]; 
//2412 ASICII字符集点阵
extern const unsigned char font_2412[95][36];
//3216 ASICII字符集点阵
extern const unsigned char font_3216[95][64];
//4824 数码管字体ASICII字符集点阵
extern const unsigned char font_4824[95][144];


enum {
    FONT_16=0,
    FONT_24,
    FONT_32,
    FONT_48,

    FONT_MAX
};

typedef struct {
    U8  width;
    U8  height;
    U8  size;
}font_info_t;


font_info_t font_get_info(U8 font);

const U8* font_get_data(U8 chr, U8 font);

#endif

