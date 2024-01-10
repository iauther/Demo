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

///////////////////
enum {
    CH_0=0,
    CH_1,
    
    CH_MAX
};

enum {
    EV_RMS=0,
    EV_AMP,
    EV_ASL,
    EV_ENE,
    EV_AVE,         //average
    EV_MIN,
    EV_MAX,
    
    EV_NUM
};

enum {
    FL_FIR=0,
    FL_IIR,
    FL_FFT,
    
    FL_NUM
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
    PWRON_RTC=0,
    PWRON_MANUAL,
    PWRON_TIMER,
};


enum {
    PWR_NO_PWRDN=0,
    PWR_PERIOD_PWRDN,
};


typedef struct {
    F32     a;
    F32     b;
}coef_t;

enum {
    SMP_MODE_PERIOD=0,
    SMP_MODE_TRIG,
    SMP_MODE_CONT,
};

typedef struct {
    U32             preTime;            //unit: us
    U32             postTime;           //unit: us
    
    U32             PDT;                /*trigger PEAK DEFINTTION TIME,   unit: us
                                          PDT为峰值定义时间, 在PDT的时间内出现的最大峰值认为是波峰, PDT的功能是为了快速,
                                          准确的找到AE波形的主峰值的同时避免将前驱波误判为主波
                                        */
    
    U32             HDT;                /*trigger HIT DEFINTTION TIME,    unit: us
                                          HDT为撞击定义时间, 在HDT的时间内不再出现过阈值, 则认为该撞击结束, 
                                          结束之后就开始存储声发射波形和计算特征参数
                                        */
    
    U32             HLT;                /*trigger HIT LOCKOUT TIME,       unit: us
                                          HLT为撞击锁闭时间, 在HLT的时间内即使出现过阈值也不认为是一个撞击信号, 
                                          撞击锁闭时间是为了避免将反射波和滞后波当成主波处理
                                        */
    
    U32             MDT;                /*trigger MAX DURATION TIME       unit: us
                                          MDT为最大持续时间, 当信号比较密集时，超过此长度的数据将会倍强行截断
                                        */
}trig_time_t;
typedef struct {
    U8              ch;
    U8              enable;
    U32             smpFreq;            //hz
    U8              smpMode;            //SMP_PERIOD_MODE: period sample  SMP_TRIG_MODE: threshold trigger sample   SMP_CONT_MODE: continuous sample
    
    //period sample param
    U32             smpPoints;
    U32             smpTimes;           //number of times
    U32             smpInterval;        //sample interval time, unit: us
    
    //trigger sample param
    U8              trigEvType;         //EV_RMS~EV_MAX    
    F32             trigThreshold;      //ev threshold value,   unit: mv
    trig_time_t     trigTime;
    
    //ev param
    U8              n_ev;
    U8              ev[EV_NUM];
    U32             evCalcPoints;
    
    U8              upway;              //0: upload realtime   1: delay upload together
    U8              upwav;
    U8              savwav;

    coef_t          coef;
}ch_para_t;

typedef struct {
    U8              mode;               //work mode
    U8              port;               //refer to PORT_UART define
    
    U8              pwrmode;           //PWR_NO_PWRDN:no powerdown    PWR_PERIOD_PWRDN: period powerdown
    U32             workInterval;      //unit: second
    ch_para_t       ch[CH_MAX];
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
    U32             idx;
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
    datetime_t      dt;
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
}upg_info_t;

typedef struct {
    U32             head;            //UPG_HEAD_MAGIC
    U32             upgCnt;
    U32             fwAddr;          //app fw header offset in flash
    U32             facFillFlag;     //0: not fill  1: already fill
    U32             upgFlag;         //UPG_FLAG_NONE etc.
    U32             runFlag;         //
    U32             tail;            //UPG_TAIL_MAGIC
}upg_data_t;

typedef struct {
    fw_info_t       fw;
    upg_info_t      upg;
    U8              data[0];
}fw_hdr_t;

typedef struct {
    upg_data_t     upg;
    fw_hdr_t       hdr;
}upg_all_t;


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
    U8              psrc;       //poweron trig source, 0: manual poweron  1: rtc poweron  2: timer poweron
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

