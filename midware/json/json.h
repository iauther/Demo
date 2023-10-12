#ifndef __JSON_Hx__
#define __JSON_Hx__

#include "protocol.h"

int json_init(void);
int json_from(char *js, int js_len, usr_para_t *para);
int json_to(char *js, usr_para_t *para);

#endif

