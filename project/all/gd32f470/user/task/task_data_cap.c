#include "task.h"
#include "dal_delay.h"
#include "log.h"
#include "list.h"
#include "cfg.h"
#include "json.h"
#include "ads9120.h"

#include "fs.h"


#define RX_LEN  1000
#define TX_LEN  10

typedef struct {
    U16  rx[RX_LEN];
    int  rxLen;
    
    
    U16  tx[TX_LEN];
    
    U16  ox[RX_LEN/DAC_FREQ_DIV];
}my_buf_t;


#ifdef OS_KERNEL
my_buf_t myBuf={0};

static void ads_data_callback(U16 *data, U32 len)
{
    node_t node={data,len,len};

    task_post(TASK_DATA_CAP, NULL, EVT_ADS, 0, &node, sizeof(node));
}
static void vib_data_callback(U8 *data, U32 len)
{
    node_t node={data,len,len};

    task_post(TASK_DATA_CAP, NULL, EVT_ADC, 0, &node, sizeof(node));
}
static void tmr_data_callback(U8 *data, U32 len)
{
    node_t node={data,len,len};
    
    //task_post(TASK_DATA_CAP, NULL, EVT_TIMER, 0, &node, sizeof(node));
}

static int ads_init(void)
{
    int r;
    ads9120_cfg_t ac;
    
    ac.buf.rx.buf  = (U8*)myBuf.rx;
    ac.buf.rx.blen = sizeof(myBuf.rx);
    ac.buf.tx.buf  = (U8*)myBuf.tx;
    ac.buf.tx.blen = sizeof(myBuf.tx);
    ac.buf.ox.buf  = (U8*)myBuf.ox;
    ac.buf.ox.blen = sizeof(myBuf.ox);
    ac.freq = 2500*KHZ;
    ac.callback = ads_data_callback;
    
    r = ads9120_init(&ac);
    
    return r;
}
static int vib_init(void)
{
    return 0;
}




static void cap_config(void)
{
    ads_init();
    vib_init();
    
}

void task_data_cap_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;    
    
    LOGD("_____ task_data_cap running\n");
    
    cap_config();
    
    while(1) {
        r = task_recv(TASK_DATA_CAP, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                case EVT_ADC:
                {
                    LOGD("___ EVT_ADC\n");
                }
                break;
                
                case EVT_ADS:
                {
                    LOGD("___ EVT_ADS\n");
                }
                break;
                
                case EVT_TIMER:
                {
                    node_t *nd=(node_t*)e.data;
                    //
                }
                break;
            }
        }
    }
}
#endif














