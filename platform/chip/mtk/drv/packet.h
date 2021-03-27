#ifndef __PACKET_Hx__
#define __PACKET_Hx__

#include "types.h"
#include "bio/bio.h"

#pragma pack (1)
typedef struct {
	U16         head;                  //0x1bdf;
	U8          flag;                  //命令标识, 1
	U8          cmdLen;                //包ID号
    
    //U8        cmdData[];
    //U8        chkSum;
    //U16       tail;                 //0x9bdf
}cmd_hdr_t;

typedef struct {
	U16         head;                  //0x1bdf;
	U8          ver;                   //版本, 1
	U16         pid;                   //包ID号
	U8          chs;                   //通道数
	U8          format;                //数据排列方式 1= 112233,2= 123123
	U8          bits;                  //数据位数
	U8          lsb;                   //0=msb，1=lsb
	bio_stat_t  stat;                 //0
	U16         ecgLen;                //ecg数据长度
    
    //U8        ecgData[];
    //U8        chkSum;
    //U16       tail;                 //0x9bdf
}data_hdr_t;

#pragma pack ()


#define CMD_HDR_SIZE                sizeof(cmd_hdr_t)
#define CMD_PACKET_SIZE(datalen)    (CMD_HDR_SIZE+datalen+3)

#define DATA_HDR_SIZE               sizeof(data_hdr_t)
#define DATA_PACKET_SIZE(datalen)   (DATA_HDR_SIZE+datalen+3)




int packet_cmd_fill(U8 *buf, U16 bufLen, U8 *cmd, U16 cmdLen);
int packet_data_fill(U8 *buf, U16 bufLen, U8 *data, U16 dataLen, U8 chs, U8 bits);

#endif


