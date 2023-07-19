#ifndef __PROTOCOL_Hx__
#define __PROTOCOL_Hx__

#include "types.h"

#define FW_MAGIC            0xFACEBEAD
#define PKT_MAGIC           0xDEADBEEF
#define UPG_HEAD_MAGIC      0xABCD1234
#define UPG_TAIL_MAGIC      0x1234ABCD

enum {
    OBJ_BOOT=0,
    OBJ_APP,
};

enum {
    UPG_FLAG_NORMAL=0,
    UPG_FLAG_START,
    UPG_FLAG_FINISH,
    
    UPG_FLAG_MAX=0x0f,
};


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
    TYPE_CAP,       //channel capture data
    TYPE_ACK,
    TYPE_SETT,
    TYPE_PARA,
    TYPE_ERROR,
    TYPE_FILE,
    TYPE_TIME,
    TYPE_HBEAT,
    
    TYPE_MAX
};


enum {
    FILE_CFG=0,
    FILE_CAL,
    FILE_SETT,
    
    FILE_LOG,
    FILE_APP,
    FILE_BOOT,
    
    FILE_MAX
};


#pragma pack (1)

typedef struct {
    U8  type;
    
    
}ev_cfg_t;

typedef struct {
    U8  chID;
    F32 caliParaA;
    F32 caliParaB;
    F32 coef;
    
    U32 freq;
    U32 sampleTime;
    U32 samplePoints;
}ch_cfg_t;

typedef struct {
    U32             len;   //ch_data_t total length, including the data length
    U8              ch;
    U32             sr;    //samplerate
    date_time_t     tm;

    U32             data[0];
}ch_data_t;

typedef struct {
    U8              cmd;
    U32             para;
}cmd_t;

typedef struct {
    U8              mode;
    
}sett_t;

typedef struct {
    U8              sysState;
}state_t;

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
    U16             pkts;
    U16             pid;
    U16             dataLen;
    U8              data[];
}file_pkt_t;

typedef struct {
    U32             magic;           //FW_MAGIC
    U8              version[20];     //V10.00.00
    U8              bldtime[20];     //20220810_162208
    U32             length;          
}fw_info_t;                          
                                     
typedef struct {                     
    U8              obj;             //0: boot       1: app
    U8              force;           
    U8              erase;           //0: no erase   1: do erase
    U8              flag;
    U32             jumpAddr;
    U8              padd[72];
}upg_ctl_t;

typedef struct {
    U32             head;            //UPG_HEAD_MAGIC
    U32             upgCnt;
    U32             fwAddr;          //app fw header offset in flash
    U32             runAddr;         //the boot jump offset
    U32             facFillFlag;     //0: not fill  1: already fill
    U32             upgFlag;         //UPG_FLAG_NONE etc.
    U32             tail;            //UPG_TAIL_MAGIC
}upg_info_t;

//make sure sizeof(upg_hdr_t)==128 bytes
typedef struct {
    fw_info_t       fwInfo;
    upg_ctl_t       upgCtl;
    U8              data[0];
}upg_hdr_t;

typedef struct {
    U8              digit[32];
}md5_t;

typedef struct {
    state_t         stat;
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

