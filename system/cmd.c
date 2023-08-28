#include "cmd.h"
#include <string.h>


const char *cmdStr[CMD_NUM]={
    "ECXX:",
    "USER:",
    
};



int cmd_get(char *data, cmd_t *cmd)
{
    int i,r=-1,len;
    char tmp[CMD_HDR_MAX_LEN+1];
    
    if(!data) {
        return -1;
    }
    
    len = strlen(data);
    if(len>=CMD_MAX_LEN) {
        return -1;
    }
    
    memset(tmp, 0, sizeof(tmp));
    memcpy(tmp, data, CMD_HDR_MAX_LEN);
    for(i=0; i<CMD_NUM; i++) {
        if(strstr(tmp, cmdStr[i])) {
            r = i;
        }
    }
    if(r>=0) {
        if(cmd) {
            char *ptr = data+strlen(cmdStr[r]);
            strcpy(cmd->str, ptr);
            
            cmd->ID = r;
        }
    }
    
    return (r<0)?-1:0;
}




