#ifndef __ESP8266_H
#define __ESP8266_H 			   
#include "platform.h"
#include "types.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

//ESP8266模式选择
typedef enum
{
    MODE_STA=0,
    MODE_AP,
    MODE_APSTA  
}net_mode_t;

typedef enum{
     PROT_TCP=0,
     PROT_UDP,
}net_prot_t;

//连接号，指定为该连接号可以防止其他计算机访问同一端口而发生错误
typedef enum{
    MULTI_ID_0 = 0,
    MULTI_ID_1 = 1,
    MULTI_ID_2 = 2,
    MULTI_ID_3 = 3,
    MULTI_ID_4 = 4,
    SINGLE_ID_0 = 5,
}net_id_t;



#define FRAME_MAX_LEN 512       //最大字节数
typedef struct {
    U8 rxBuf[FRAME_MAX_LEN];
    U8 txBuf[FRAME_MAX_LEN];
    union {
        __IO U16 InfAll;
        struct 
        {
            __IO U16 FramLength       :15;                               // 14:0 
            __IO U16 FramFinishFlag   :1;                                // 15 
        }InfBit;
    }; 
}esp8266_data_t;





#endif
