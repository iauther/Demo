#include "task.h"
#include "list.h"
#include "paras.h"
#include "dal_adc.h"
#include "cfg.h"
#include "dsp.h"
#include "rtc.h"
#include "mem.h"
#include "dal_delay.h"


//状态量：描述物质状态的物理量，       如：位置、速度/加速度、动量、动能、角速度、角动量、压强、温度、体积、势能等
//过程量：描述物质状态变化过程的物理量，如：冲量、功、热量、速度改变量等都是过程量，它与时间变化量相对应

#define IEQ(x1,x2)  (abs((int)x1-(int)x2)==0)
#define FEQ(x1,x2)  (fabs((F32)x1-(F32)x2)<0.000001f)


#ifdef OS_KERNEL

#define US(points,sfreq) ((U64)1000000*(points)/(sfreq))
#define MS(points,sfreq) ((U64)1000*(points)/(sfreq))
#define POINTS(us,sfreq)    ((U64)us*sfreq/1000000)


static void fmemcpy(F32 *m1, F32 *m2, int cnt)
{
    memcpy(m1, m2, cnt*4);
}
static void fmemmove(F32 *m1, F32 *m2, int cnt)
{
    memmove(m1, m2, cnt*4);
}
static void threshold_reset(threshold_t *thr)
{
    thr->idx = -1;
    thr->max = 0.0f;
    thr->trigged = 0;
    
    thr->cur.pdt = -1;
    thr->cur.hdt = -1;
    thr->cur.hlt = -1;
    thr->cur.mdt = -1;
    
    thr->pre.dcnt = 0;
    thr->bdy.dcnt = 0;
    thr->post.dcnt = 0;
}
static inline int threshold_is_full(threshold_buf_t *b)
{
    return (b->dcnt==b->bcnt);
}
static inline int threshold_fill(threshold_buf_t *b, F32 v)
{
    if(b->dcnt>=b->bcnt) {
        return -1;
    }
    b->data[b->dcnt++] = v;
    
    return 0;
}

enum {
    THR_FULL_FIFO=0,
    THR_FULL_FILO,
};
static inline int threshold_copy(threshold_buf_t *b, F32 *v, int cnt, int mode)
{
    if(cnt<=0) {
        return -1;
    }
    
    if(b->dcnt+cnt>b->bcnt) {
        int cnt1,cnt2;
        
        if(mode==THR_FULL_FIFO) {       //丢最老数据
            
            if(cnt>b->bcnt) {       //新数据多于内存总长, 截取新数据尾部bcnt数据
                fmemcpy(b->data, v+cnt-b->bcnt, b->bcnt);
                b->dcnt = b->bcnt;
            }
            else {
                if(cnt+b->dcnt>b->bcnt) {
                    cnt1=b->dcnt+cnt-b->bcnt;   //原数据最前面要丢的个数
                    cnt2=b->dcnt-cnt1;          //原数据尾部剩下的数据
                    
                    fmemmove(b->data, b->data+cnt1, cnt2); //将原数据尾部的cnt2个数据搬到头部
                    fmemcpy(b->data+cnt2, v, cnt);         //将新数据接在尾部
                    b->dcnt = b->bcnt;
                }
                else {
                    fmemcpy(b->data+b->dcnt, v, cnt);
                    b->dcnt += cnt;
                }
            }
        }
        else {      //丢最新的数据
            cnt1=b->bcnt-b->dcnt;
            if(cnt1>0) {
                fmemcpy(b->data+b->dcnt, v, cnt1);
                b->dcnt += cnt1;
            }
        }
    }
    else {
        fmemcpy(b->data+b->dcnt, v, cnt);
        b->dcnt += cnt;
    }
    
    return 0;
}

static inline int threshold_send(threshold_t *thr, ch_para_t *para, U8 mdtfull)
{
    int i,j,tcnt=0,ev_calc_cnt,evlen=0;
    int r,postcnt=0,tlen;
    buf_t *buf=&taskBuffer.var[para->ch].prc;
    ch_data_t *pch=(ch_data_t*)buf->buf;
    
    fmemcpy(pch->data+tcnt, thr->pre.data, thr->pre.dcnt);
    tcnt += thr->pre.dcnt;
    
    fmemcpy(pch->data+tcnt, thr->bdy.data, thr->bdy.dcnt);
    tcnt += thr->bdy.dcnt;
    
    if(!mdtfull) {
        fmemcpy(pch->data+tcnt, thr->post.data, thr->post.dcnt);
        tcnt += thr->post.dcnt;
        postcnt = thr->post.dcnt;
    }
    
    if(tcnt==0) {
        return -1;
    }
    
    pch->ch = para->ch;
    pch->time = thr->time-MS(thr->pre.dcnt, para->smpFreq);
    pch->smpFreq = para->smpFreq;
    pch->wavlen = tcnt*4;
    
    ev_data_t *pev=(ev_data_t*)(pch->data+tcnt);
    pev->grps = 1;
    
    ev_calc_cnt = tcnt;
    evlen = sizeof(ev_data_t)+pev->grps*sizeof(ev_grp_t);
    for(i=0; i<pev->grps; i++) {
        ev_grp_t *pgrp=&pev->grp[i];
        
        pgrp->time = pch->time+MS(ev_calc_cnt*i, para->smpFreq);
        pgrp->cnt  = para->n_ev;
        for(j=0; j<pgrp->cnt; j++) {
            ev_val_t *val=&pgrp->val[j];
            val->tp = para->ev[j];
            dsp_ev_calc(val->tp, pch->data+thr->pre.dcnt, thr->bdy.dcnt, &val->data);
        }
        evlen += pgrp->cnt*sizeof(ev_val_t);
    }
    pch->evlen = evlen;
    tlen = sizeof(ch_data_t)+pch->wavlen+pch->evlen;
    
    LOGD("___trigger, mdtfull:%d, ch:%d, trigged:%d, idx:%d, pre:%d, bdy:%d, post:%d, pdt:%d, hdt:%d, hlt:%d, mdt:%d, tcnt: %d, tlen: %d\n", 
          mdtfull, pch->ch, thr->trigged, thr->idx, thr->pre.dcnt, thr->bdy.dcnt, postcnt, thr->cur.pdt, thr->cur.hdt, thr->cur.hlt, thr->cur.mdt, tcnt, tlen);
    
    r = api_send_append(pch, tlen);
    if(r==0) {
        thr->trigged = 1;
        thr->pre.dcnt = 0;
        thr->bdy.dcnt = 0;
        thr->post.dcnt = 0;
        
        memset(pch, 0, sizeof(ch_data_t));
    }
    else {
        LOGE("___ threshold_send, append failed\n");
    }
    
    return r;
}

static int period_proc(raw_data_t *raw, ch_para_t *para)
{
    int i,j,r;
    int tlen=0;
    task_buf_t *tb=&taskBuffer;
    int smp_len=para->smpPoints*sizeof(raw_t);
    int ev_calc_len=para->evCalcPoints*sizeof(raw_t);
    int t_skp_len=para->smpInterval*(para->smpFreq*sizeof(raw_t)/1000);
    ch_var_t *cv=&tb->var[raw->ch];
    ch_data_t  *pch=(ch_data_t*)cv->prc.buf;
    int data_len=raw->cnt*4;
    
    raw_t *real_data=raw->data;
    int    real_len=data_len;
    
    if(real_len>=ev_calc_len) {
            
        //填充暂存数据头部ch_data_t
        pch->ch = raw->ch;
        pch->time = raw->time;
        pch->smpFreq = para->smpFreq;
        pch->wavlen = real_len;
        pch->evlen  = 0;
        memcpy(pch->data, real_data, pch->wavlen);      //拷贝wav数据
        
        if(para->n_ev>0) {
            ev_data_t *pev=(ev_data_t*)(pch->data+pch->wavlen/4);
            
            ev_grp_t *grp=pev->grp;
            pev->grps = real_len/ev_calc_len;
            
            for(i=0; i<pev->grps; i++) {
                U32 index=i*para->evCalcPoints;
                grp->time = pch->time+index*1000/para->smpFreq;
                grp->cnt = para->n_ev;
                
                for(j=0; j<grp->cnt; j++) {
                    ev_val_t *val=&grp->val[j];
                    val->tp = para->ev[j];
                    dsp_ev_calc(val->tp, real_data+index, para->evCalcPoints, &val->data);
                }
                grp = (ev_grp_t*)((U8*)grp + sizeof(ev_grp_t) + sizeof(ev_val_t)*grp->cnt);
            }
            
            pch->evlen = sizeof(ev_data_t)+pev->grps*(sizeof(ev_grp_t)+para->n_ev*sizeof(ev_val_t));
            
            ev_data_t *pev2 = (ev_data_t*)((U8*)pch->data+pch->wavlen);
            memcpy(pev2, pev, pch->evlen);  //在wav数据后面追加ev_data_t数据
        }

        tlen = sizeof(ch_data_t)+pch->wavlen+pch->evlen;
    }
    else { //如果计算ev的数据个数比较多，大于一次采集的数据, 则进行缓存后再计算
        
        if(pch->wavlen>0 && pch->time>0) {    //已经有缓冲数据
            if(pch->wavlen+real_len>=ev_calc_len){
                int xlen=ev_calc_len-pch->wavlen;
                memcpy((U8*)pch->data+pch->wavlen, real_data, xlen);
                pch->wavlen += xlen;
            }
            else {
                memcpy((U8*)pch->data+pch->wavlen, real_data, real_len);
                pch->wavlen += real_len;
            }
        }
        else { //空数据，需要填充头部
            pch->ch = raw->ch;
            pch->time = raw->time;
            pch->smpFreq = para->smpFreq;
            pch->wavlen = 0;
            
            memcpy((U8*)pch->data, real_data, real_len);
            pch->wavlen += real_len;
        }
        
        //数据缓冲完成则进行计算
        if(pch->wavlen>=ev_calc_len) {
            ev_data_t *pev=(ev_data_t*)(pch->data+pch->wavlen/4);
            
            ev_grp_t *grp=pev->grp;
            pev->grps = 1;
            
            grp->time = pch->time;
            grp->cnt = para->n_ev;
            
            ev_val_t *val=grp->val;
            for(j=0; j<para->n_ev; j++) {
                val[j].tp = para->ev[j];
                dsp_ev_calc(val[j].tp, pch->data, para->evCalcPoints, &val[j].data);
            }
            
            pch->evlen = sizeof(ev_data_t)+1*(sizeof(ev_grp_t)+para->n_ev*sizeof(ev_val_t));
            memcpy((U8*)pch->data+pch->wavlen, pev, pch->evlen);  //在wav数据后面追加ev数据
            
            tlen = sizeof(ch_data_t)+pch->wavlen+pch->evlen;
        }
    }

    if(tlen>0) {
        //LOGD("_______________ RMS: %f\n", ((ev_data_t*)((U8*)(p_ch->data)+p_ch->wavlen))->grp[0].val[0].data);
        r = api_send_append(pch, tlen);
        
        if(r==0) {
            memset(pch, 0, sizeof(ch_data_t));
        }
    }
    
    return r;
}
//返回0时，数据需后处理
static int trig_proc(raw_data_t *raw, ch_para_t *para)
{
    F32 x,max=0.0f;
    int i,j,r=-1,tlen=0;
    ch_var_t *var=&taskBuffer.var[raw->ch];
    threshold_t *thr=&var->thr;
    ch_data_t *pch=(ch_data_t*)var->prc.buf;

#if 0
    pch->ch = raw->ch;
    pch->time = raw->time;
    pch->smpFreq = para->smpFreq;
    pch->wavlen = 0;
    pch->evlen = sizeof(ev_data_t)+1*(sizeof(ev_grp_t)+para->n_ev*sizeof(ev_val_t));
    
    ev_data_t *pev=(ev_data_t*)(pch->data+pch->wavlen/4);
    ev_grp_t *grp=pev->grp;
    
    pev->grps = 1;
    grp->time = raw->time;
    grp->cnt = para->n_ev;
    
    ev_val_t *val=grp->val;
    for(j=0; j<para->n_ev; j++) {
        val[j].tp = para->ev[j];
        dsp_ev_calc(val[j].tp, pch->data, raw->cnt, &val[j].data);
    }
    memcpy((U8*)pch->data+pch->wavlen, pev, pch->evlen);  //在wav数据后面追加ev数据
    
    tlen = sizeof(ch_data_t)+pch->evlen;
    r = api_send_append(pch, tlen);
    if(r==0) {
        memset(pch, 0, sizeof(ch_data_t));
    }
#endif
    
    for(i=0; i<raw->cnt; i++) {
        if(thr->max>0) {
            thr->idx++;
        }
        
        x = fabs(raw->data[i]);
        //if(x>0.05f) LOGD("__cur__x: %0.4f\n", x);
        if(x>=thr->vth) {
            
            if(thr->max==0.0f) {
                thr->idx = 0;
                thr->cur.mdt = thr->idx;
                thr->time = rtc2_get_timestamp_ms()+MS(i,para->smpFreq);
                
                threshold_copy(&thr->pre, raw->data, i, THR_FULL_FIFO);     //保留最新数据，丢前面老数据
            }
            
            if(x>thr->max) {
                thr->max = x;
                thr->cur.pdt = thr->idx;
                thr->cur.hdt = -1;
            }
        }
        else {
            if(thr->cur.hdt==-1) {
                thr->cur.hdt = thr->idx;
            }
        }
        
        if(thr->cur.pdt>=0) {
            if(thr->idx-thr->cur.pdt>=thr->set.pdt) {
                if(thr->cur.hdt==-1) {
                    thr->cur.hdt = thr->idx;
                }
                else {
                    if(thr->idx-thr->cur.hdt>=thr->set.hdt) {                  
                        if(thr->cur.hlt==-1) {
                            thr->cur.hlt = thr->idx;
                        }
                        else {
                            //当数据长度超过HLT设置时，则需将数据丢弃
                            if(thr->idx-thr->cur.hlt>=thr->set.hlt) {
                                threshold_reset(thr);   //复位变量, 重新开始新一轮
                                continue;
                            }
                        }
                        
                        if(thr->trigged==0) {
                            if(threshold_is_full(&thr->post)) {
                                threshold_send(thr, para, 0);
                            }
                            else {
                                threshold_fill(&thr->post, raw->data[i]);
                            }
                        }
                        
                        continue;
                    }
                    else {
                        if(x>=thr->vth) {
                            thr->cur.hdt = -1;
                        }
                    }
                }
                
                //当数据长度超过MDT设置时，则需将数据截断
                if(thr->cur.mdt>=0 && (thr->idx-thr->cur.mdt)>=thr->set.mdt) {
                    if(thr->cur.hlt==-1) {
                        thr->cur.hlt = thr->idx;
                    }
                    else {
                        //当数据长度超过HLT设置时，则需将数据丢弃
                        if(thr->idx - thr->cur.hlt >= thr->set.hlt) {
                            threshold_reset(thr);   //复位变量, 重新开始新一轮
                        }
                    }
                    
                    //达到最大采集长度, 后续数据截断而非丢弃
                    if(!thr->trigged) {
                        threshold_send(thr, para, 1);
                    }

                    continue;
                }
            }
            
            threshold_fill(&thr->bdy, raw->data[i]);
            
            //LOGD("__cur__x: %0.4f, vth: %0.4f, idx: %d pdt: %d, hdt: %d, hlt: %d, mdt: %d\n", x, Vth, thr->idx, thr->cur.pdt, thr->cur.hdt, thr->cur.hlt, thr->cur.mdt);
            //LOGD("__set__%d pdt: %d, hdt: %d, hlt: %d, mdt: %d\n", thr->idx, thr->set.pdt, thr->set.hdt, thr->set.hlt, thr->set.mdt);
        }
    }
    
    if(thr->max==0.0f) {
        thr->pre.dcnt=0;
        threshold_copy(&thr->pre, raw->data, raw->cnt, THR_FULL_FIFO);
    }
    
    return r;
}



static int data_proc(void *data, int len)
{
    int r;
    raw_data_t *raw=(raw_data_t*)data;
    ch_para_t *para=paras_get_ch_para(raw->ch);

    if(para->smpMode==SMP_MODE_PERIOD) {
        r = period_proc(raw, para);
    }
    else if(para->smpMode==SMP_MODE_TRIG) {
        r = trig_proc(raw, para);          //利用特征值计算结果
    }
            
    //LOGD("_________ cost ms: %lld\n", dal_get_tick_ms()-ts);
    
    return r;
}


static int ch_data_proc(void)
{
    int ch,r=-1;
    list_node_t *lnode=NULL;
    ch_var_t *var=taskBuffer.var;
    
    for(ch=0; ch<CH_MAX; ch++) {
        r = list_take_node(var[ch].raw, &lnode, 0);      
        if(r==0) {
            //LOGD("___ size: %d\n", list_size(var[ch].raw));
            data_proc(lnode->data.buf, lnode->data.dlen);
            list_back_node(var[ch].raw, lnode);
        }
    }
    
    return r;
}
////////////////////////////////////////////////////////////////////
int api_data_proc_send(U8 ch, void *data, int len)
{
    int r=list_append(taskBuffer.var[ch].raw, 0, data, len);
    if(r==0) {
        r = task_trig(TASK_DATA_PROC, EVT_DATA);
    }
    else {
        LOGD("___ api_data_proc_send failed\n");
    }
    
    return r;
}



void task_data_proc_fn(void *arg)
{
    int r;
    evt_t e;
    
    LOGD("_____ task data proc running\n");
    
    while(1) {
        
        r = task_recv(TASK_DATA_PROC, &e, sizeof(e));
        if(r==0) {
             switch(e.evt) {
                case EVT_DATA:
                {
                    ch_data_proc();
                    //print_ts("222");
                }
                break;
            }
        }
    }
}
#endif


