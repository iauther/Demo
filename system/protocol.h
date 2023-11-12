#ifndef __PROTOCOL_Hx__
#define __PROTOCOL_Hx__

#include "types.h"
#include "dsp.h"
#include "dal_adc.h"

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
    PORT_UART=0,
    PORT_NET,
    PORT_USB,

    PORT_MAX
};

enum {
    MODE_NORM=0,
    MODE_CALI,
    MODE_TEST,
    MODE_DEBUG,

    MODE_MAX
};

enum {
    STAT_STOP=0,
    STAT_RUN,
};

enum {
    LEVEL_RMS=0,
    LEVEL_VPP,
    
};


enum {
    TYPE_ACK=0,
    
    TYPE_CAP,         //capture data
    TYPE_CALI,        //calibration
    TYPE_STAT,
    TYPE_DAC,
    TYPE_MODE,
    TYPE_SETT,
    TYPE_PARA,
    TYPE_FILE,
    TYPE_TIME,
    
    TYPE_DFLT,
    TYPE_REBOOT,
    TYPE_FACTORY,
    
    TYPE_HBEAT,       //heartbeat
    TYPE_DATO,
    
    TYPE_MAX
};

enum {
    FILE_PARA=0,
    
    FILE_CAP,       //adc data
    FILE_UPG,
    FILE_LOG,
    
    FILE_MAX
};

enum {
    PROTO_TCP=0,
    PROTO_UDP,
    PROTO_MQTT,
    PROTO_COAP,
};

enum {
    DATO_ALI=0,
    DATO_USR,
};

enum {
    DATA_LT=0,
    DATA_WAV,
    DATA_SETT,
};




#ifdef _WIN32
#pragma warning(disable : 4200)
#endif

#pragma pack (1)

typedef F32     raw_t;
typedef F32     ev_t;

typedef struct {
    char ip[40];
    U16  port;
}tcp_para_t;
typedef struct {
    char host[100];
    U16  port;
    char prdKey[20];
    char prdSecret[64];
    char devKey[32];
    char devSecret[64];
}plat_para_t;
typedef struct {
    U8   mode;              //0: tcp, 1: plat
    union {
        tcp_para_t  tcp;
        plat_para_t plat;
    }para;
}net_para_t;
typedef struct {
    U8   addr;
}mbus_para_t;
typedef struct {
    U8   type;
    U8   auth;              //0: None 1: PAP  2: CHAP
    char apn[20];
}card_para_t;
typedef struct {
    U8   to;
    U8   level;
    U8   enable;
}dbg_para_t;
typedef struct {
    U8   enable;
    U32  fdiv;              //freq divider
}dac_para_t;

enum {
    PWR_NO_PWRDN=0,
    PWR_PERIOD_PWRDN,
};


typedef struct {
    F32     a;
    F32     b;
}coef_t;

enum {
    SMP_PERIOD_MODE=0,
    SMP_TRIG_MODE,
    SMP_CONT_MODE,
};

typedef struct {
    U8      smpMode;        //SMP_PERIOD_MODE: period sample  SMP_TRIG_MODE: threshold trigger sample   SMP_CONT_MODE: continuous sample
    U32     smpFreq;        //hz
    U32     smpPoints;
    U32     smpInterval;    //sample interval time, unit: us
    U32     smpTimes;       //number of times
    F32     ampThreshold;   //sample AMP threshold value, unit: mv
    U32     messDuration;   //messure end duration, unit: ms
    U32     trigDelay;      //trigger delay time,   unit: ms
    U8      ev[EV_NUM];
    U8      n_ev;
    U8      upway;          //0: upload realtime   1: delay upload together
    U8      upwav;
    U8      savwav;
    U32     evCalcCnt;
}ch_para_t;
typedef struct {
    U8              ch;
    U8              enable;
    
    ch_para_t       para[MODE_MAX];
    coef_t          coef;
}ch_paras_t;
typedef struct {
    U8              mode;               //dev mode
    U8              port;               //refer to PORT_UART define
    
    U8              pwrmode;           //PWR_NO_PWRDN:no powerdown    PWR_PERIOD_PWRDN: period powerdown
    U32             worktime;          //unit: second
    ch_paras_t      ch[CH_MAX];
}smp_para_t;

typedef struct {
    net_para_t  net;
    dac_para_t  dac;
    dbg_para_t  dbg;
    mbus_para_t mbus;
    card_para_t card;
    smp_para_t  smp;
}usr_para_t;

typedef struct {
    U32             tp;
    F32             data;
}ev_val_t;

typedef struct {
    U64             time;
    U32             cnt;
    ev_val_t        val[0];
}ev_grp_t;

typedef struct {
    U32             grps;
#ifndef _WIN32
    ev_grp_t        grp[0];
#endif
}ev_data_t;

typedef struct {
    U32             ch;
    U64             time;
    U32             smpFreq;
    U32             wavlen;
    U32             evlen;
    F32             data[0];        //wavdata+evdata
}ch_data_t;

typedef struct {
    S32             rssi;          //dB,
    S32             ber;           //0~7,-1
    F32             vbat;          //batt volt
    F32             temp;
    U64             time;
}stat_data_t;

typedef struct {
    U8              ch;
    U8              enable;
}capture_t;

typedef struct {
    U32             cntdown;
}powerdown_t;

typedef struct {
    U8              xx;           //
}sett_t;

typedef struct {
    U8          proto;
    net_para_t  *para;
    rx_cb_t     callback;
}conn_para_t;

typedef struct {
    S8              stat[CH_MAX];
    U8              finished[CH_MAX];
    date_time_t     dt;
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
    U32             devID;
    U8              type;
    U8              flag;
    U8              askAck;         //1: need ack     0: no need ack
    U16             dataLen;
    U8              data[0];
    //U8            checksum;
}pkt_hdr_t;

/////////////upgrade define////////////////////
typedef struct {
    U8              ftype;
    U16             pkts;
    U16             pid;
    U16             dataLen;
    U8              data[0];
}file_hdr_t;

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
    U32             devID;
    //U32           type;
}dev_info_t;

typedef struct {
    fw_info_t       fwInfo;
    dev_info_t      devInfo;
    sett_t          sett;
}sys_para_t;

typedef struct {
    U8              max;        //1~10
    U8              seq;        //1~10
    U8              lv;         //0:rms, 1:vpp, unit:mv
    U8              ch;         //0~CH_MAX-1
    F32             volt;       //mv
    U32             freq;
    F32             bias;       //mv
}cali_sig_t;

#define CALI_MAX    5
typedef struct {
    F32             in;
    F32             out;
}cali_rms_t;
typedef struct {
    U32             cnt;
    cali_rms_t      rms[CALI_MAX];
    cali_sig_t      sig;
}cali_t;
typedef struct {
    U8              psrc;       //poweron trig source, 0: manual poweron  1: rtc poweron
    dac_para_t      dac;
    cali_t          cali[CH_MAX];
    state_t         state;
}gbl_var_t;

typedef struct {
    gbl_var_t       var;
    usr_para_t      usr;
    sys_para_t      sys;
}all_para_t;

#pragma pack ()

#define PKT_HDR_LENGTH      sizeof(pkt_hdr_t)


#endif

