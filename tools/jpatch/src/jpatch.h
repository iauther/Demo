#ifndef _JPATCH_Hx__
#define _JPATCH_Hx__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t     *buf;
    long        size;       /* buffer �ĳ��� */
    long        pos;        /* 0���ļ���ͷ��size���ļ���β*/
}jp_data_t;


int jpatch(jp_data_t *source, jp_data_t *patch, jp_data_t *target);


#endif // _JANPATCH_H
