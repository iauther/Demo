#include "task.h"
#include "list.h"
#include "fs.h"
#include "pkt.h"
#include "rtc.h"
#include "cfg.h"
#include "mem.h"
#include "comm.h"
#include "paras.h"





#ifdef OS_KERNEL

static buf_t sbuf;
int api_comm_send_data(U8 type, U8 nAck, void *data, int len)
{
    int xlen = pkt_pack_data(type, nAck, data, len, sbuf.buf, sbuf.blen, 0);
    if(xlen<=0) {
        return -1;
    }
    
    return list_append(taskBuffer.send, 0, sbuf.buf, xlen);
}
int api_comm_send_ack(U8 type, U8 err)
{
    int xlen = pkt_pack_ack(type, err, sbuf.buf, sbuf.blen, 0);
    if(xlen<=0) {
        return -1;
    }
    
    return list_append(taskBuffer.send, 0, sbuf.buf, xlen);
}


void task_comm_send_fn(void *arg)
{
    int r,dlen;
    list_node_t *lnode=NULL;
    
    sbuf.blen = 1000*20;
    sbuf.buf  = eMalloc(sbuf.blen);
    sbuf.dlen = 0;
    
    while(1) {    
        if(list_take_node(taskBuffer.send, &lnode, 0)==0) {
            
            U8 dato=allPara.usr.smp.dato;
            ch_data_t *pch=(ch_data_t*)lnode->data.buf;
            ch_para_t *p=paras_get_ch_para(pch->ch);
            
            if(p->savwav && !p->upwav) {
                pch->wavlen = 0;
                memcpy(pch->data, (U8*)pch->data+pch->wavlen, pch->evlen);
            }
            
            dlen = sizeof(ch_data_t)+pch->wavlen+pch->evlen;
            r = comm_send_data(tasksHandle.hconn, &dato, TYPE_CAP, 0, pch, dlen);
            if(r) {
                LOGE("___ task_comm_send, comm_send_data failed\n");
            }
            //comm_pure_send(tasksHandle.hconn, &to, lnode->data.buf, lnode->data.dlen);
            
            list_back_node(taskBuffer.send, lnode);
        }
        
        
        
        
        osDelay(1);
    }
}
#endif


