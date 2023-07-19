#include "task.h"
#include "list.h"
#include "comm.h"
#include "xmem.h"
#include "dal_adc.h"
#include "cfg.h"
//#include "alg.h"


//״̬������������״̬����������       �磺λ�á��ٶ�/���ٶȡ����������ܡ����ٶȡ��Ƕ�����ѹǿ���¶ȡ���������ܵ�
//����������������״̬�仯���̵����������磺�����������������ٶȸı����ȶ��ǹ�����������ʱ��仯�����Ӧ



#define VDD_APPLI           ((U32) 3300)    /* Value of analog voltage supply Vdda (unit: mV) */
#define RANGE_8BITS         ((U32)  255)    /* Max digital value with a full range of 8 bits */
#define RANGE_12BITS        ((U32) 4095)    /* Max digital value with a full range of 12 bits */
#define RANGE_16BITS        ((U32)65535)    /* Max digital value with a full range of 16 bits */

#define S_ADCTO(v)          ((3.3F/65535)*(v))
#define D_ADCTO(v)          (((2.0F*(v))/65535-1)*3.3F)

typedef struct {
    U8          ch;
    ch_cfg_t    *cfg;
}arg_t;



int list_cov_get(U8 ch, node_t *node)
{
    return list_get(tasksHandle.covList[ch], node, 0);
}
void list_cov_remove(U8 ch)
{
    list_remove(tasksHandle.covList[ch], 0);
}
void list_cov_add(U8 ch, void *data, int len)
{    
    list_append(tasksHandle.covList[ch], data, len);
}




void adc_data_conv(U32 *src, F32 *dst, int cnt)
{
    int i;
    
    for(i=0; i<cnt; i++) {
        dst[i] = D_ADCTO(src[i]);
    }
}

////////////////////////////////////////////////////////////
static int do_data_proc(U8 ch, ch_cfg_t *cfg, U32 *data, int len)
{
    int r;
    ch_data_t *cd=tasksHandle.calBuf.buf;
    
    cd->ch = ch;
    cd->sr = 100;
    cd->len = len;
    
    //if() {
    //fir_filter();
    //ev_calc();
    if(data[0]>0xfffffff) {
        LOGD("____ ch %d over 0xfffffff\n", ch);
    }
    
    memcpy(cd->data, data, cd->len);
    r = list_append(tasksHandle.sendList, cd, sizeof(ch_data_t)+cd->len);
    
    return r;
}


static int proc_iterator_fn(handle_t l, node_t *data, node_t *xd, void *p, int *act)
{
    arg_t *arg=(arg_t*)p;
    
    *act = ACT_REMOVE;
    return do_data_proc(arg->ch, arg->cfg, (U32*)xd->buf, xd->dlen);
}

static int ch_data_proc(U8 ch, ch_cfg_t *cfg)
{
    int r=0;
    arg_t arg={ch,cfg};
    handle_t list=tasksHandle.oriList[ch];
    
    if(list) {
        r = list_iterator(list, NULL, proc_iterator_fn, &arg);
    }
    
    return r;
}





#ifdef OS_KERNEL
extern void comm_send_trigger(void);
void task_data_proc_fn(void *arg)
{
    int ch,r;
    U8  err;
    evt_t e;
    node_t node;
    task_handle_t *h=(task_handle_t*)arg;
    
    LOGD("_____ task_data_proc running\n");
    while(1) {
        r = task_recv(TASK_DATA_PROC, &e, sizeof(e));
        if(r==0) {
            for(ch=0; ch<CH_MAX; ch++) {
                r = ch_data_proc(ch, &tasksHandle.cfg[ch]);
                if(r) {
                    //LOGD("____ ch %d data add failed\n", ch);
                }
            }
            
            comm_send_trigger();
        }
    }
}
#endif


