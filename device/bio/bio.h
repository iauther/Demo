#ifndef __BIO_Hx__
#define __BIO_Hx__

#include "types.h"
#include "bio/ad8233.h"
#include "bio/ads129x.h"

enum{
    AD8233=0,
    ADS129X,
};

enum {
    DC=0,
    AC,
};

enum {
    LINE_1P=0,
    LINE_1N,
    LINE_2P,
    LINE_2N,
    LINE_3P,
    LINE_3N,
    LINE_4P,
    LINE_4N,
    LINE_5P,
    LINE_5N,
    LINE_6P,
    LINE_6N,
    LINE_7P,
    LINE_7N,
    LINE_8P,
    LINE_8N,
    
    LINE_MAX,
};

enum {
    ET_LA=0,        //ET: electrode
    ET_RA,
    ET_LL,
    ET_RLD,
    ET_V1,
    ET_V2,
    ET_V3,
    ET_V4,
    
    ET_MAX,
    ET_WCT,
};

enum {
    LEAD_I=0,
    LEAD_II,
    LEAD_III,
    LEAD_V1,
    LEAD_V2,
    LEAD_V3,
    LEAD_V4,
    LEAD_RLD,
    
    LEAD_MAX
};


/////////////electrode switch define start//////////////
enum {
    CMD_SEL_RLD_SRC=0,
    CMD_SEL_DIY_SRC,
    CMD_SEL_WCT_SRC,
    CMD_SET_LOFF_CURRENT,
    CMD_DEV_RESET,
    CMD_SET_FAST_RECOVER,
    CMD_SET_LOFF_PULLUP,
};

enum {      //CMD_SEL_RLD_SRC
    RLD_SRC_I=0,
    RLD_SRC_II,
    RLD_SRC_III,
    RLD_SRC_DIY,                        // ID_RLD_DIY_FB + ID_RLD_DIY_WCT
};
enum {
    RLD_DIY_SRC_FB_I=(1<<0),
    RLD_DIY_SRC_FB_II=(1<<1),
    RLD_DIY_SRC_FB_III=(1<<2),
    RLD_DIY_SRC_WCT=(1<<3),
};
enum {
    RLD_DIY_SRC_WCT_RA=(1<<0),
    RLD_DIY_SRC_WCT_LA=(1<<1),
    RLD_DIY_SRC_WCT_LL=(1<<2),
};
enum {
    MESSURE_LEAD_NONE=0,
    MESSURE_LEAD_NORMAL,
    MESSURE_LEAD_RLD,
};
/////////////electrode switch define end//////////////

#pragma pack (1)
typedef struct {
    U8      dev;                    //AD8233 or ADS129X
    U32     mvValue;
}bio_info_t;

typedef struct {
    U8       in[LINE_MAX];
    U8       out[LINE_MAX];
}rld_set_t;

typedef struct {
    U8       et[ET_MAX];
}wct_set_t;

typedef struct {
    U8        bits;                 //adc有效位数
    S32       adc[LEAD_MAX];
}ecg_data_t;

typedef struct {
    U8        loff;                 //0: on     1: off
}loff_data_t;

typedef struct {
    U8          current;              //loff detect current, 0: DC   1: AC
    wct_set_t   wct;
    rld_set_t   rld;
}bio_setting_t;

typedef struct {
    U8          current;            //脱落检测电流，默认交流
    
    loff_data_t loff;
}bio_stat_t;

typedef struct {
    U8          id;                 //见上面定义
    U8          para;
}bio_cmd_t;


typedef void (*bio_callback)(void *user_data);
typedef struct {
    int             freq;
    bio_callback    ecgCallback;
    bio_callback    loffCallback;
    void            *user_data;
}bio_callback_t;

#pragma pack ()


typedef struct {
    int (*init)(void);
    int (*reset)(void);
    int (*get_stat)(bio_stat_t *stat);
    int (*send_cmd)(bio_cmd_t *cmd);
    int (*start)(void);
    int (*stop)(void);
    int (*get_info)(bio_info_t *info);
    int (*set_current)(U8 current);
    int (*get_setting)(bio_setting_t *sett);
    int (*set_callback)(bio_callback_t *cb);
    
    int (*read_ecg)(ecg_data_t *ecg);
    int (*read_loff)(loff_data_t *loff);
    int (*get_rld)(rld_set_t *rld);
    int (*set_rld)(rld_set_t *rld);
    int (*set_wct)(wct_set_t *wct);
    int (*get_wct)(wct_set_t *wct);
}bio_handle_t;

extern volatile int bioDevice;
bio_handle_t *bio_probe(void);

#endif
