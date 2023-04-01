#include "task.h"
#include "list.h"




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

static list_cap_t capList={0};
void list_cap_init(void)
{
    int i;
    list_cfg_t cfg;
    list_data_t *pStat=&capList.stat;
    list_data_t *pProc=&capList.proc;
    
    cfg.mode = MODE_FIFO;
    cfg.max = 10;
    pStat->list = list_new(&cfg);
    pProc->bufLen = 2000*4;
    pProc->buf = malloc(pProc->bufLen);
    
    cfg.mode = MODE_FIFO;
    cfg.max = 10;
    pProc->list = list_new(&cfg);
    pProc->bufLen = 2000*4;
    pProc->buf = malloc(pProc->bufLen); 
}

void list_cap_add(int id, void *data, int len)
{
    list_data_t *pStat=&capList.stat;
    list_data_t *pProc=&capList.proc;
    
    if(id==0) {
        list_append(pStat->list, data, len);
    }
    else {
        list_append(pProc->list, data, len);
    }
}
void list_cap_get(int id, node_t *node)
{
    list_data_t *pStat=&capList.stat;
    list_data_t *pProc=&capList.proc;
    
    if(id==0) {
        list_get(pStat->list, node, 0);
    }
    else {
        list_get(pProc->list, node, 0);
    }
    
}
void list_cap_remove(int id)
{
    list_data_t *pStat=&capList.stat;
    list_data_t *pProc=&capList.proc;
    
    if(id==0) {
        list_remove(pStat->list, 0);
    }
    else {
        list_remove(pProc->list, 0);
    }
}




static inline void stat_data_conv(U8 dual, U32 *buf, U32 count)
{
    int i,r;
    node_t node;
    U16 *ptr=(U16*)buf;

#if 0
    r = list_get(stat->list, &node, 0);
    if(r==0) {
        cap1->ch = 0;
        cap2->ch = 1;
        for(i=0; i<count; i++) {
            if(dual) {
                cap1->data[i] = D_ADCTO(buf[i]&0xffff);
                cap2->data[i] = D_ADCTO(buf[i]>>16);
            }
            else {
                cap1->data[i] = D_ADCTO(ptr[i]);
                cap2->data[i] = D_ADCTO(ptr[i+1]);
            }
        }
        
        stat->dataLen = 9999;
        
        list_remove(stat->list, 0);
    }
#endif
    
}


typedef struct {
    int    flag;
}data_handle_t;


#ifdef OS_KERNEL
void task_dataproc_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    task_handle_t *h=(task_handle_t*)arg;
    
    //list_cap_init();
    
    
    while(1) {
        r = task_recv(TASK_DATAPROC, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_DATA:
                {
                    //list_get(proc->list, data, len);
                    //stat_data_conv(ADC_DUAL_MODE, (U32*)nd->ptr, nd->len/4);
                    //if(myHandle.netAddr) {
                    //    err = com_send_data(myHandle.hcom, myHandle.netAddr, EVT_ADC, 0, myHandle.stat.buf, myHandle.stat.dataLen);
                    //    err = com_pure_send(myHandle.hcom, myHandle.netAddr, n.ptr, n.len);
                    //}
                }
                break;                
                
                default:
                continue;
            }
        }
    }
}
#endif


