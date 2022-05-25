#ifndef __DATA_TESTBOX_Hx__
#define __DATA_TESTBOX_Hx__

#include "types.h"
#include "led.h"
#include "buzzer.h"
#include "nvm.h"

#include "ad9834.h"





#define FW_MAGIC            0xFACEBEAD
#define UPG_MAGIC           0xABCD1234
#define PKT_MAGIC           0xDEADBEEF

//#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)


enum {
    STAT_STOP=0,
    STAT_RUNNING,
    STAT_UPGRADE,
    
    STAT_MAX
};



enum {
    TYPE_CMD=0,
    TYPE_STAT,
    TYPE_ACK,
    TYPE_SETT,
    TYPE_PARA,
    TYPE_ERROR,
    TYPE_UPGRADE,
    TYPE_LEAP,
    
    TYPE_MAX
};


enum {
    IO_LV_INPUT=0,
    IO_LV_OUTPUT,       //level
    IO_INT_OUTPUT,      //interrupt
    
    IO_VT_OUTPUT,       //voltage
    IO_UART_INPUT,
    IO_UART_OUTPUT,
};


#pragma pack (1)
typedef struct {
    U8              cmd;
    U32             para;
}cmd_t;



typedef struct {
    U8              model[10];  //EDCXXX ...
    F32             volt;
    struct {
        U8          R;
        U8          O;
        U8          Z;
    }port;
}module_para_t;


typedef struct {
    ad9834_para_t   ad9834;
    module_para_t   module;
}sett_t;


typedef struct {
    U8 pwr_mode;                //0: manual, 1: program
    
    struct {
        F32     m_volt;         //main voltage
        F32     n_volt;         //noise voltage
        F32     m_cur;          //module current
    }power;
    
    struct {
        U32     duration;       //ms
        F32     times;
    }trig; 
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

