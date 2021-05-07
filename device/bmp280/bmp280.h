#ifndef __BMP280_Hx__
#define __BMP280_Hx__

#include "types.h"
#include "bmp280/bmp280_ori.h"

typedef struct {
    F32  pres;      //kpa
    F32  temp;
}bmp280_t;

int bmp280_init(void);
int bmp280_deinit(void);
int bmp280_get(bmp280_t *bmp);

#endif

