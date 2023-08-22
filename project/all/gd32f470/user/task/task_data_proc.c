#include "task.h"
#include "list.h"
#include "comm.h"
#include "paras.h"
#include "dal_adc.h"
#include "cfg.h"
#include "dsp.h"
#include "rtc.h"
#include "dal_delay.h"


//状态量：描述物质状态的物理量，       如：位置、速度/加速度、动量、动能、角速度、角动量、压强、温度、体积、势能等
//过程量：描述物质状态变化过程的物理量，如：冲量、功、热量、速度改变量等都是过程量，它与时间变化量相对应



#ifdef OS_KERNEL


typedef struct {
    U8          ch;
    adc_para_t  *para;
}arg_t;


int list_cov_get(U8 ch, node_t *node)
{
    return list_get(listBuffer.cov[ch], node, 0);
}
void list_cov_remove(U8 ch)
{
    list_remove(listBuffer.cov[ch], 0);
}
void list_cov_add(U8 ch, void *data, int len)
{    
    list_append(listBuffer.cov[ch], data, len);
}


////////////////////////////////////////////////////////////
static U32 procLength=0;
static F32 ev_tmp[100];
static int data_proc_fn(U8 ch, adc_para_t *para, F32 *data, int len)
{
    F32 *ev;
    int i,j,r,times;
    int tlen=0;
    
    cap_data_t *pcap=(cap_data_t*)data;
    ch_data_t *ptmp=(ch_data_t*)listBuffer.prc.buf;
    U8 *pdata=(U8*)ptmp->data;
    
    times = len/(para->evCalcLen*4);
    
    ptmp->ch = ch;
    ptmp->id = ch;                  //临时用ch代替id
    ptmp->time = pcap->time;
    ptmp->dlen = 0;
    ptmp->evlen = times*para->n_ev*4;

    if(para->upwav) {
        ptmp->dlen = len*4;
        memcpy(pdata, pcap->data, ptmp->dlen);
    }
    
    ev = ev_tmp;
    if(para->n_ev>0) {
        for(i=0; i<times; i++) {
            for(j=0; j<para->n_ev; j++) {
                dsp_ev_calc((EV_TYPE)para->ev[j], pcap->data+i*para->evCalcLen, para->evCalcLen, para->smpFreq, ev+j);
            }
            
            ev += para->n_ev;
        }
        memcpy(pdata+ptmp->dlen, ev_tmp, ptmp->evlen);
    }
    
    tlen = ptmp->dlen+ptmp->evlen;
    if(tlen>0) {
        tlen += sizeof(ch_data_t);
        r = list_append(listBuffer.send, ptmp, tlen);
        if(r) {
            LOGE("___add to send failed\n");
        }
    }
    
    return r;
}


static int proc_iterator_fn(handle_t l, node_t *data, node_t *xd, void *p, int *act)
{
    arg_t *arg=(arg_t*)p; *act = ACT_REMOVE;
    
    procLength += xd->dlen;
    return data_proc_fn(arg->ch, arg->para, (F32*)xd->buf, xd->dlen);
}

static int ch_data_proc(U8 ch, adc_para_t *para)
{
    int r=0;
    arg_t arg={ch,para};
    
    r = list_iterator(listBuffer.ori[ch], NULL, proc_iterator_fn, &arg);
    
    return r;
}


void task_data_proc_fn(void *arg)
{
    int ch,r;
    U8  err;
    adc_para_t *pch=allPara.usr.ch;
    
    LOGD("_____ task data proc running\n");
    
    while(1) {
        
        for(ch=0; ch<CH_MAX; ch++) {
            r = ch_data_proc(ch, &pch[ch]);
        }
        
        osDelay(1);
    }
}
#endif


