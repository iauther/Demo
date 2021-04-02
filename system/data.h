#ifndef __DATA_Hx__
#define __DATA_Hx__

#include "types.h"
#include "n950.h"
#include "ms4525.h"
#include "valve.h"
#include "bmp280/bmp280.h"

#define FW_MAGIC            0xFACEBEAD
#define PKT_MAGIC           0xDEADBEEF

//#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

enum {
    MODE_CONTINUS=0,        //连续模式
    MODE_INTERVAL,          //间歇模式
    MODE_FIXED_TIME,        //固定时长
    MODE_FIXED_VOLUME,      //固定吸量
    
    MODE_MAX
};

enum {
    STAT_AUTO=0,
    STAT_MANUAL,
    STAT_UPGRADE,
    
    STAT_MAX
};


enum {
    TYPE_CMD=0,
    TYPE_STAT,
    TYPE_ACK,
    TYPE_SETT,
    TYPE_PARAS,
    TYPE_ERROR,
    TYPE_UPGRADE,
    TYPE_LEAP,
    
    TYPE_MAX
};

enum {
    CMD_PUMP_SPEED=0,
    CMD_VALVE_SET,
    
    CMD_SYS_RESTART,
    CMD_SET_FACTORY,
};

#pragma pack (1)
typedef struct {
    U8              cmd;
    U32             para;
}cmd_t;

typedef struct {
    U8              mode;
    struct {
        U16         work_time;      //ms
        U16         stop_time;      //ms
        U16         total_time;     //ms
    }time;
    F32             pres;           //differential pressure, unit: kPa
    F32             maxVol;         //the max volume, unit: ml
}sett_t;

typedef struct {
    U8              mode;
    sett_t          sett[MODE_MAX];
}setts_t;

typedef struct {
    U8              stat;
    F32             temp;        //environment temperature,   unit: degree celsius
    F32             aPres;       //outside absolute pressure, unit: kpa
    F32             dPres;       //differential pressure,     unit: kPa
    
    n950_stat_t     pump;
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
    //U8              checksum;
}pkt_hdr_t;

/////////////upgrade define////////////////////
typedef struct {
    U32             magic;          //FW_MAGIC
    U8              version[20];    //v1.0.0
    U8              bldtime[20];    //2021.02.06
}fw_info_t;

typedef struct {
    U8              obj;           //0: boot   1: app
    U16             pkts;
    U16             pid;
    U16             dataLen;
    U8              data[];
}upgrade_pkt_t;

typedef struct {
    U8              force;
    U8              erase;          //0: not erase   1: do erase
    U8              action;         //0: do nothing  1: restart   0x3f: run app
}upgrade_ctl_t;

typedef struct {
    U8              md5[32];
    upgrade_ctl_t   upgCtl;
    fw_info_t       fwInfo;
    U8*             data[];
}upgrade_file_hdr_t;

typedef struct {
    fw_info_t       fwInfo;
    setts_t         setts;
}paras_t;

#pragma pack ()


#define PKT_HDR_LENGTH      sizeof(pkt_hdr_t)

typedef struct {
    struct {
        int         enable;     //timeout enable
        int         retries;
    }set[TYPE_MAX];
}ack_timeout_t;

extern U8 curState;
extern paras_t curParas;
extern ack_timeout_t ackTimeout;



#endif

