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



#define TRIG_MAX   100
typedef struct {
    U8  ov;   //0: lower than threshold  1: over threshold
    int inx;
    int len;
}trig_node_t;
typedef struct {
    int         num;
    trig_node_t node[TRIG_MAX];
}trig_data_t;


static trig_data_t trigData={.num=0};

static int find_hl_threshold(F32 *data, int cnt, F32 v, int hl)
{
    int i;
    for(i=0; i<cnt; i++) {
        if(hl==0) {
            if(fabs(data[i])>v) {
                return i-1;
            }
        }
        else {
            if(fabs(data[i])<v) {
                return i-1;
            }
        }
    }
    
    return 0;
}


typedef struct {
    int   dir;        //0: 低于阈值   1: 高于阈值
    int   off;
}thr_node_t;

#define THR_MAX  1000
typedef struct {
    int              num;
    thr_node_t node[THR_MAX];
}thr_data_t;
static thr_data_t thrData={0};
     

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
    
    thr->cur.pdt = -1;
    thr->cur.hdt = -1;
    thr->cur.hlt = -1;
    thr->cur.mdt = -1;
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
    if(b->dcnt+cnt>b->bcnt) {
        int cnt1,cnt2;
        
        if(mode==THR_FULL_FIFO) {
            
            if(cnt>b->bcnt) {       //新数据多余内存总长
                fmemcpy(b->data, v+cnt-b->bcnt, b->bcnt);
                b->dcnt = b->bcnt;
            }
            else {
                if(cnt+b->dcnt>b->bcnt) {
                    cnt1=b->dcnt+cnt-b->bcnt;   //最前面要丢的个数
                    cnt2=b->dcnt-cnt1;          //剩下数据的个数
                    
                    fmemmove(b->data, b->data+cnt1, cnt2);
                    fmemcpy(b->data+cnt2, v, cnt);
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
                fmemcpy(b->data+b->bcnt, v, cnt1);
                b->bcnt += cnt1;
            }
        }
    }
    else {
        fmemcpy(b->data+b->bcnt, v, cnt);
        b->bcnt += cnt;
    }
    
    return 0;
}

static inline int threshold_send(threshold_t *thr, ch_para_t *para, U8 all)
{
    int tcnt=0;
    int r,prelen,postlen,bodylen,tlen;
    buf_t *buf=&taskBuffer.var[para->ch].prc;
    ch_data_t *pch=(ch_data_t*)buf->buf;
    
    fmemcpy(pch->data+tcnt, thr->pre.data, thr->pre.dcnt);
    tcnt += thr->pre.dcnt;
    thr->pre.dcnt = 0;
    
    fmemcpy(pch->data+tcnt, thr->bdy.data, thr->bdy.dcnt);
    tcnt += thr->bdy.dcnt;
    thr->bdy.dcnt = 0;
    
    if(!all) {
        fmemcpy(pch->data+tcnt, thr->post.data, thr->post.dcnt);
        tcnt += thr->post.dcnt;
    }
    thr->post.dcnt = 0;
    
    pch->ch = para->ch;
    pch->time = thr->time;
    pch->smpFreq = para->smpFreq;
    pch->wavlen = tcnt*4;
    pch->evlen = 0;   
    
    
    tlen = sizeof(ch_data_t)+pch->wavlen;
    r = api_nvm_send(pch, tlen);
    
    return r;
}



//返回0时，数据需后处理
static int amp_trig_proc(raw_data_t *raw, ch_para_t *para)
{
    F32 x,max=0.0f;
    int i,r=-1;
    threshold_t *thr=&taskBuffer.var[raw->ch].thr;
    F32 Vth=pow(10,para->trigThreshold/20-3);
    
    for(i=0; i<raw->cnt; i++) {
        
        if(thr->max>0) {
            thr->idx++;
        }
        
        x = fabs(raw->data[i]);
        //if(x>0.05f) LOGD("__cur__x: %0.4f\n", x);
        if(x>=Vth) {
            
            if(thr->max==0.0f) {
                thr->idx = 0;
                thr->cur.mdt = thr->idx;
                thr->time = rtc2_get_timestamp_ms()+MS(i+1,para->smpFreq);
                
                threshold_copy(&thr->pre, raw->data, i, THR_FULL_FIFO);     //保留最新数据，丢前面老数据
            }
            
            if(x>thr->max) {
                thr->max = x;
                thr->cur.pdt = thr->idx;  
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
                        
                        if(threshold_is_full(&thr->post)) {
                            threshold_send(thr, para, 0);
                        }
                        else {
                            threshold_fill(&thr->post, raw->data[i]);
                        }
                        
                        if(thr->cur.hlt==-1) {
                            thr->cur.hlt = thr->idx;
                        }
                        else {
                            //当数据长度超过HLT设置时，则需将数据丢弃
                            if(thr->idx-thr->cur.hlt<thr->set.hlt) {
                                //数据丢弃
                                continue;
                            }
                            else {
                                threshold_reset(thr);   //复位变量, 重新开始新一轮
                            }
                        }
                    }
                    else {
                        if(x>=Vth) {
                            thr->cur.hdt = -1;
                        }
                    }
                }
                
                //当数据长度超过MDT设置时，则需将数据截断
                if(thr->idx-thr->cur.mdt>=thr->set.mdt) {
                    //达到最大采集长度, 后续数据截断而非丢弃
                    threshold_send(thr, para, 1);
                    threshold_reset(thr);
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


static int data_proc_fn(handle_t l, U8 ch, F32 *data, int len)
{
    int i,j,r;
    int tlen=0;
    task_buf_t *tb=&taskBuffer;
    smp_para_t *smp=paras_get_smp();
    ch_para_t *para=paras_get_ch_para(ch);
    int smp_len=para->smpPoints*sizeof(raw_t);
    int ev_calc_len=para->evCalcPoints*sizeof(raw_t);
    int t_skp_len=para->smpInterval*(para->smpFreq*sizeof(raw_t)/1000);
    raw_data_t *p_raw=(raw_data_t*)data;
    ch_var_t *cv=&tb->var[ch];
    ch_data_t  *p_ch=(ch_data_t*)cv->prc.buf;
    ev_data_t *p_ev=(ev_data_t*)tb->var[ch].ev.buf;
    ev_data_t *p_ev2=NULL;
    int data_len=len-sizeof(raw_data_t);
    
    raw_t *real_data=p_raw->data;
    int    real_len=data_len;
    
    if(real_len>=ev_calc_len) {
        
        //填充暂存数据头部ch_data_t
        p_ch->ch = ch;
        p_ch->time = p_raw->time;
        p_ch->smpFreq = para->smpFreq;
        p_ch->wavlen = real_len;
        memcpy(p_ch->data, real_data, p_ch->wavlen);      //拷贝wav数据
        
        if(para->n_ev>0) {
            ev_grp_t *grp=p_ev->grp;
            p_ev->grps = real_len/ev_calc_len;
            
            for(i=0; i<p_ev->grps; i++) {
                U32 idx,index=i*para->evCalcPoints;
                grp->time = p_ch->time+i*para->evCalcPoints*1000/para->smpFreq;
                grp->cnt = para->n_ev;
                
                for(j=0; j<grp->cnt; j++) {
                    ev_val_t *val=&grp->val[j];
                    val->tp = para->ev[j];
                    idx = dsp_ev_calc(val->tp, real_data+i*para->evCalcPoints, para->evCalcPoints, &val->data);
                    val->idx = index+idx;
                }
                grp = (ev_grp_t*)((U8*)grp + sizeof(ev_grp_t) + sizeof(ev_val_t)*grp->cnt);
            }
            
            p_ch->evlen = sizeof(ev_data_t)+p_ev->grps*(sizeof(ev_grp_t)+para->n_ev*sizeof(ev_val_t));
            
            p_ev2 = (ev_data_t*)((U8*)p_ch->data+p_ch->wavlen);
            memcpy(p_ev2, p_ev, p_ch->evlen);  //在wav数据后面追加ev_data_t数据
        }
        
        tlen = p_ch->wavlen+p_ch->evlen;
    }
    else { //如果计算ev的数据个数比较多，大于一次采集的数据, 则进行缓存后再计算
        
        if(p_ch->wavlen>0 && p_ch->time>0) {    //已经有缓冲数据
            if(p_ch->wavlen+real_len>=ev_calc_len){
                int xlen=ev_calc_len-p_ch->wavlen;
                memcpy((U8*)p_ch->data+p_ch->wavlen, real_data, xlen);
                p_ch->wavlen += xlen;
            }
            else {
                memcpy((U8*)p_ch->data+p_ch->wavlen, real_data, real_len);
                p_ch->wavlen += real_len;
            }
        }
        else { //空数据，需要填充头部
            p_ch->ch = ch;
            p_ch->time = p_raw->time;
            p_ch->smpFreq = para->smpFreq;
            p_ch->wavlen = 0;
            
            memcpy((U8*)p_ch->data, real_data, real_len);
            p_ch->wavlen += real_len;
        }
        
        //数据缓冲完成则进行计算
        if(p_ch->wavlen>=ev_calc_len) {
            
            ev_grp_t *grp=p_ev->grp;
            p_ev->grps = 1;
            
            grp->time = p_ch->time;
            grp->cnt = para->n_ev;
            
            ev_val_t *val=grp->val;
            for(j=0; j<para->n_ev; j++) {
                val[j].tp = para->ev[j];
                val[j].idx = dsp_ev_calc(val[j].tp, p_ch->data, para->evCalcPoints, &val[j].data);
            }
            
            p_ch->evlen = sizeof(ev_data_t)+1*(sizeof(ev_grp_t)+para->n_ev*sizeof(ev_val_t));
            memcpy((U8*)p_ch->data+p_ch->wavlen, p_ev, p_ch->evlen);  //在wav数据后面追加ev数据
            
            tlen = p_ch->wavlen+p_ch->evlen;
        }
    }
          
    if(tlen>0) {
        
        //LOGD("_______________ RMS: %f\n", ((ev_data_t*)((U8*)(p_ch->data)+p_ch->wavlen))->grp[0].val[0].data);
        tlen += sizeof(ch_data_t);
        //r = api_nvm_send(p_ch, tlen);
        
        if(para->smpMode==SMP_MODE_TRIG) {
            buf_t *buf=&cv->prc;
            threshold_t *thr=&cv->thr;
            
            r = amp_trig_proc(p_raw, para);          //利用特征值计算结果
            if(r) {
                return -1;
            }

            p_ch->ch = ch;
            p_ch->time = thr->time;
            p_ch->smpFreq = para->smpFreq;
            p_ch->wavlen = buf->dlen;
            p_ch->evlen = 0;
            
            tlen = sizeof(ch_data_t)+p_ch->wavlen;
            r = api_nvm_send(p_ch, tlen);
            
            LOGD("____ trig send len %d, rslt: %d\n", tlen, r);
        }
        
        if(r==0) {
            memset(p_ch, 0, sizeof(ch_data_t));
        }
    }
    
    return r;
}


static int ch_data_proc(void)
{
    int ch,r=0;
    list_node_t *lnode=NULL;
    ch_var_t *var=taskBuffer.var;
    
    for(ch=0; ch<CH_MAX; ch++) {
        r = list_take_node(var[ch].raw, &lnode, 0);      
        if(r==0) {
            data_proc_fn(var[ch].raw, ch, lnode->data.buf, lnode->data.dlen);
            list_back_node(var[ch].raw, lnode);
        }
    }
    
    return r;
}




void task_data_proc_fn(void *arg)
{
    int r;
    
    LOGD("_____ task data proc running\n");
    
    while(1) {
        
        ch_data_proc();
        
        osDelay(1);
    }
}
#endif


