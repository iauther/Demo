#ifndef __PARA_Hx__
#define __PARA_Hx__

#include "data.h"
#include "notice.h"

#define ACK_POLL_MS     300


typedef struct {
    struct {
        U8          enable;
        int         resendIvl;       //resend interval time,  unit: ms
        int         retryTimes;      //retry max
    }set[TYPE_MAX];
}ack_timeout_t;


int paras_load(void);
int paras_erase(void);
int paras_reset(void);
int paras_read(U32 addr, void *data, U32 len);
int paras_write(U32 addr, void *data, U32 len);
int paras_write_node(node_t *n);

int paras_get_fwmagic(U32 *fwmagic);
int paras_set_fwmagic(U32 *fwmagic);
int paras_get_fwinfo(fw_info_t *fwinfo);
int paras_set_fwinfo(fw_info_t *fwinfo);
int paras_set_upg(void);
int paras_clr_upg(void);


extern U8 adjMode;
extern U8 sysState;
extern U8 sysMode;
extern stat_t curStat;
extern paras_t curParas;
extern ack_timeout_t ackTimeout;
extern notice_t allNotice[LEV_MAX];
extern paras_t DEFAULT_PARAS;

#endif

