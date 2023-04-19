#include "task.h"
#include "list.h"
#include "com.h"
#include "xmem.h"
#include "cfg.h"


//状态量：描述物质状态的物理量，       如：位置、速度/加速度、动量、动能、角速度、角动量、压强、温度、体积、势能等
//过程量：描述物质状态变化过程的物理量，如：冲量、功、热量、速度改变量等都是过程量，它与时间变化量相对应


U64 sdramBuffer[XMEM_CAP_LEN/8] __attribute__ ((at(XMEM_CAP_ADDR)));


#define VDD_APPLI           ((U32) 3300)    /* Value of analog voltage supply Vdda (unit: mV) */
#define RANGE_8BITS         ((U32)  255)    /* Max digital value with a full range of 8 bits */
#define RANGE_12BITS        ((U32) 4095)    /* Max digital value with a full range of 12 bits */
#define RANGE_16BITS        ((U32)65535)    /* Max digital value with a full range of 16 bits */

#define S_ADCTO(v)          ((3.3F/65535)*(v))
#define D_ADCTO(v)          (((2.0F*(v))/65535-1)*3.3F)


typedef struct {
    handle_t        list;
    U32             *buf;
    int             bufLen;
    int             dataLen;
}list_data_t;

typedef struct {
    list_data_t     stat;
    list_data_t     proc;
}list_cap_t;

typedef struct {
    void            *buf;
    int             bufLen;
    int             dataLen;
}tmp_buf_t;

typedef struct {
    list_cap_t      capList;
    tmp_buf_t       tmp;
    handle_t        hmem;
}dp_handle_t;


static dp_handle_t dpHandle;
void list_cap_init(void)
{
    int i;
    list_cfg_t cfg;
    list_data_t *pStat=&dpHandle.capList.stat;
    list_data_t *pProc=&dpHandle.capList.proc;
    
    
    dpHandle.hmem = xmem_init(sdramBuffer, XMEM_CAP_LEN);
    
    cfg.mode = MODE_FIFO;
    cfg.max = 10;
    cfg.hmem = dpHandle.hmem;
    pStat->list = list_new(&cfg);
    pProc->bufLen = 2000*4;
    pProc->buf = xMalloc(dpHandle.hmem, pProc->bufLen, 0);
    
    pProc->list = list_new(&cfg);
    pProc->bufLen = 2000*4;
    pProc->buf = malloc(pProc->bufLen); 
}

void list_cap_add(int id, void *data, int len)
{
    handle_t list;
    list_data_t *pStat=&dpHandle.capList.stat;
    list_data_t *pProc=&dpHandle.capList.proc;
    
    list = (id==EVT_DATA_STAT)?pStat->list:pProc->list;
    list_append(list, data, len);
}
int list_cap_get(int id, node_t *node)
{
    handle_t list;
    list_data_t *pStat=&dpHandle.capList.stat;
    list_data_t *pProc=&dpHandle.capList.proc;
    
    list = (id==EVT_DATA_STAT)?pStat->list:pProc->list;
    return list_get(list, node, 0);
}
void list_cap_remove(int id)
{
    handle_t list;
    list_data_t *pStat=&dpHandle.capList.stat;
    list_data_t *pProc=&dpHandle.capList.proc;
    
    list = (id==EVT_DATA_STAT)?pStat->list:pProc->list;
    list_remove(list, 0);
}




static inline void stat_data_conv(U8 dual, U32 *buf, U32 count)
{
    int i,r;
    node_t node;
    U16 *ptr=(U16*)buf;
    int dlen=dual?(count*4):(count*2);
    ch_data_t *cd1=(ch_data_t*)dpHandle.tmp.buf;
    ch_data_t *cd2=(ch_data_t*)((U8*)dpHandle.tmp.buf+dlen);

    cd1->ch = 0;
    cd2->ch = 1;
    for(i=0; i<count; i++) {
        if(dual) {
            cd1->data[i] = D_ADCTO(buf[i]&0xffff);
            cd2->data[i] = D_ADCTO(buf[i]>>16);
        }
        else {
            cd1->data[i] = D_ADCTO(ptr[i]);
            cd2->data[i] = D_ADCTO(ptr[i+1]);
        }
    }
    
    cd1->len = cd2->len = dlen;
    dpHandle.tmp.dataLen = dlen*2;
}


#ifdef OS_KERNEL
void task_dataproc_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    node_t node;
    task_handle_t *h=(task_handle_t*)arg;
    
    list_cap_init();
    while(1) {
        r = task_recv(TASK_DATAPROC, &e, sizeof(e));
        if(r==0) {
            r = list_cap_get(e.evt, &node);
            if(r==0) {
                //stat_data_conv(ADC_DUAL_MODE, (U32*)node.ptr, node.len/4);
                err = com_send_data(commHandle.hcom, commHandle.netAddr, TYPE_DATA, 0, dpHandle.tmp.buf, dpHandle.tmp.dataLen);
                //err = com_pure_send(commHandle.hcom, myHandle.netAddr, dpHandle.buf, dpHandle.dataLen);
                list_cap_remove(e.evt);
            }
                
        }
    }
}
#endif


