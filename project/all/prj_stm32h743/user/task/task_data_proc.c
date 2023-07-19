#include "task.h"
#include "list.h"
#include "comm.h"
#include "xmem.h"
#include "dal_adc.h"
#include "cfg.h"


//状态量：描述物质状态的物理量，       如：位置、速度/加速度、动量、动能、角速度、角动量、压强、温度、体积、势能等
//过程量：描述物质状态变化过程的物理量，如：冲量、功、热量、速度改变量等都是过程量，它与时间变化量相对应


U64 sdramBuffer[XMEM_LEN/8] __attribute__ ((at(XMEM_ADDR)));


#define VDD_APPLI           ((U32) 3300)    /* Value of analog voltage supply Vdda (unit: mV) */
#define RANGE_8BITS         ((U32)  255)    /* Max digital value with a full range of 8 bits */
#define RANGE_12BITS        ((U32) 4095)    /* Max digital value with a full range of 12 bits */
#define RANGE_16BITS        ((U32)65535)    /* Max digital value with a full range of 16 bits */

#define S_ADCTO(v)          ((3.3F/65535)*(v))
#define D_ADCTO(v)          (((2.0F*(v))/65535-1)*3.3F)



void list_ch_init(U32 chMask)
{
    tasksHandle.hMem = xmem_init(sdramBuffer, XMEM_LEN);
    
    tasksHandle.tbuf.blen = 2000*4;
    tasksHandle.tbuf.buf = malloc(tasksHandle.tbuf.blen); 
}


void list_ch_cfg(U32 chMask)
{
    int i;
    list_cfg_t cfg;
    
    cfg.mode = MODE_FIFO;
    cfg.hmem = tasksHandle.hMem;
    for(i=0; i<CH_MAX; i++) {
        
        if(tasksHandle.oriList[i]) {
            list_free(tasksHandle.oriList[i]);
            tasksHandle.oriList[i]=NULL;
        }
        if(tasksHandle.covList[i]) {
            list_free(tasksHandle.covList[i]);
            tasksHandle.covList[i]=NULL;
        }
        
        if(chMask&(1<<i)) {
            cfg.max = 10;
            tasksHandle.oriList[i] = list_init(&cfg);
            tasksHandle.covList[i] = list_init(&cfg);
        }
        else {
            tasksHandle.oriList[i] = NULL;
            tasksHandle.covList[i] = NULL;
        }
    }
}



void list_ch_add(U8 id, U8 ch, void *data, int len)
{
    handle_t *list=(id==0)?tasksHandle.oriList:tasksHandle.covList;
    
    list_append(list[ch], data, len);
}
int list_ch_get(U8 id, U8 ch, node_t *node)
{
    handle_t *list=(id==0)?tasksHandle.oriList:tasksHandle.covList;
    return list_get(list[ch], node, 0);
}
void list_ch_remove(U8 id, U8 ch)
{
    handle_t *list=(id==0)?tasksHandle.oriList:tasksHandle.covList;
    
    list_remove(list[ch], 0);
}

void adc_data_fill(U8 dual, U32 *buf, int len)
{
    int i,r;
    static int flag=0;
    U16 *ptr=(U16*)buf;
    int dlen=dual?(len):(len*2);
    int cnt=dual?(len/4):(len/2);
    tmp_buf_t *tb=&tasksHandle.tbuf;
    ch_data_t *cd1=(ch_data_t*)tb->buf;
    ch_data_t *cd2=(ch_data_t*)((U8*)tb->buf+dlen);
    
    for(i=0; i<cnt; i++) {
        if(dual) {
            cd1->data[i] = buf[i]&0xffff;
            cd2->data[i] = buf[i]>>16;
        }
        else {
            cd1->data[i] = ptr[i];
            cd2->data[i] = ptr[i+1];
        }
    }
    
    cd1->len = cd2->len = dlen;
    if(flag==0) {
        list_ch_add(0, CH_7,  cd1->data, cd1->len);
        list_ch_add(0, CH_8,  cd2->data, cd2->len);
    }
    else {
        list_ch_add(0, CH_9,  cd1->data, cd1->len);
        list_ch_add(0, CH_10, cd2->data, cd2->len);
    }
    
    flag = !flag;
}


static U8 bits_to_chs(U8 bits)
{
    U8 i,n=0;
    
    while(bits&1) {
        n++;  bits >>= 1;
    }
    
    return n;
}


void adc_data_conv(U32 *src, F32 *dst, int cnt)
{
    int i;
    
    for(i=0; i<cnt; i++) {
        dst[i] = D_ADCTO(src[i]);
    }
}

void sai_data_fill(U8 chBits, U32 *buf, int len)
{
    int i,r,offset;
    int tlen=0,clen;
    U16 *ptr=(U16*)buf;
    ch_data_t *cd;
    U8 chs=bits_to_chs(chBits);

    clen = len/chs;
    for(i=0; i<chs; i++) {
        offset = clen*i;
        list_ch_add(0, i, buf+offset, clen);
    }
}

////////////////////////////////////////////////////////////
static void do_data_proc(U8 ch, ch_cfg_t *cfg, U32 *data, int len)
{
    
}
static void my_data_proc(U8 ch, ch_cfg_t *cfg)
{
    int r;
    node_t node;
    
    while(1) {
        r = list_ch_get(0, ch, &node);
        if(r) {
            break;
        }
        
        do_data_proc(ch, cfg, (U32*)node.ptr, node.len);
        list_ch_remove(0, ch);
    }
}





#ifdef OS_KERNEL
extern void comm_send_trigger(void *addr);
void task_data_proc_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    node_t node;
    task_handle_t *h=(task_handle_t*)arg;
    U32 chMask=0x0fff;
    
    list_ch_init(chMask);
    while(1) {
        r = task_recv(TASK_DATA_PROC, &e, sizeof(e));
        if(r==0) {
            for(i=0; i<CH_MAX; i++) {
                my_data_proc(i, &tasksHandle.cfg[i]);
            }
            
            comm_send_trigger(tasksHandle.netAddr);
        }
    }
}
#endif


