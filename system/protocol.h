#ifndef __PROTOCOL_Hx__
#define __PROTOCOL_Hx__

#include "types.h"

#define FW_MAGIC            0xFACEBEAD
#define UPG_MAGIC           0xABCD1234
#define PKT_MAGIC           0xDEADBEEF

//#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)


enum {
    STAT_STOP=0,
    STAT_RUNNING,
    STAT_UPGRADE,
    
    STAT_POWEROFF=0x10,
    STAT_MAX
};


enum {
    TYPE_CMD=0,
    TYPE_STAT,
    TYPE_ADC,       //adc data
    TYPE_ACK,
    TYPE_SETT,
    TYPE_PARA,
    TYPE_ERROR,
    TYPE_UPGRADE,
    TYPE_LEAP,
    TYPE_TEST,
    
    TYPE_MAX
};


#pragma pack (1)


typedef struct {
    U8              ch;
    U8              mode;       //common or differential
    U32             bits;       //adc sample bits
    U32             freq;       //sample rate
    U32             time;       //micro seconds
}ch_info_t;


typedef struct {
    U8              cmd;
    U32             para;
}cmd_t;

typedef struct {
    U8              mode;
    
}sett_t;

typedef struct {
    U8              sysState;
}stat_t;

typedef struct {
    U8              type;
    U8              error;      //refer to error.h
}error_t;

typedef struct {
    U8              type;
    U8              error;      //refer to error.h
}ack_t;

typedef struct {
    U32             magic;          //PKT_MAGIC
    U8              type;
    U8              flag;
    U8              askAck;         //1: need ack     0: no need ack
    U16             dataLen;
    U8              data[];
    //U8            checksum;
}pkt_hdr_t;

/////////////upgrade define////////////////////
typedef struct {
    U32             magic;          //FW_MAGIC
    U8              version[20];    //v1.0.0
    U8              bldtime[20];    //2022.08.10
}fw_info_t;

typedef struct {
    U16             pkts;
    U16             pid;
    U16             dataLen;
    U8              data[];
}upgrade_pkt_t;

typedef struct {
    U8              goal;            //0: boot   1: app
    U8              force;
    U8              erase;          //0: not erase   1: do erase
    U8              action;         //0: do nothing  1: restart   0x3f: run app
}upgrade_ctl_t;

typedef struct {
    U8              digit[32];
}md5_t;

typedef struct {
    fw_info_t       fwInfo;
    upgrade_ctl_t   upgCtl;
    U8              data[0];
}upgrade_hdr_t;

typedef struct {
    fw_info_t       fwInfo;
    sett_t          sett;
}para_t;

typedef struct {
    U32             magic;
    U8              state;
}flag_t;

typedef struct {
    flag_t          flag;
    para_t          para;
}paras_t;

#pragma pack ()


#define PKT_HDR_LENGTH      sizeof(pkt_hdr_t)


#endif

