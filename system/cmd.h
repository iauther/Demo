#ifndef __CMD_Hx__
#define __CMD_Hx__

#include "types.h"

#define CMD_HDR_MAX_LEN     20
#define CMD_DATA_MAX_LEN    100

#define CMD_MAX_LEN         (CMD_HDR_MAX_LEN+CMD_DATA_MAX_LEN)

enum {
    CMD_ECXX=0,
    CMD_USER,
    
    CMD_MAX
};


typedef struct {
    char str[CMD_MAX_LEN];
    int  ID;
}cmd_t;


int cmd_get(char *data, cmd_t *cmd);

#endif

