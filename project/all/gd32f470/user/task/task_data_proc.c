#include "task.h"
#include "list.h"
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
    ch_para_t   *para;
}arg_t;


////////////////////////////////////////////////////////////
static ev_data_t ev_tmp[100];
static int data_proc_fn(handle_t l, U8 ch, F32 *data, int len)
{
    ev_data_t *p_ev=ev_tmp;
    int i,j,r,times;
    int tlen=0;
    U64 time;
    U32 step_ms;
    task_buf_t *tb=&taskBuffer;
    handle_t lsend=tb->send;
    smp_para_t *smp=paras_get_smp();
    ch_para_t *para=paras_get_ch_para(ch);
    
    int ev_calc_wavlen=para->evCalcCnt*sizeof(raw_t);
    int wavlen=len-sizeof(raw_data_t);
    int smplen=paras_get_smp_cnt(ch)*sizeof(raw_t);
    
    raw_data_t *p_raw=(raw_data_t*)data;
    ch_data_t  *p_ch=(ch_data_t*)tb->prc[ch].buf;
    
    step_ms = 1000/para->smpFreq;
    
    if(wavlen>=ev_calc_wavlen) {
        
        //填充暂存数据头部ch_data_t
        p_ch->ch = ch;
        p_ch->time = p_raw->time;
        p_ch->wavlen = wavlen;
        memcpy(p_ch->data, p_raw->data, p_ch->wavlen);      //拷贝wav数据
        
        if(para->n_ev>0) {
            times = wavlen/ev_calc_wavlen;
            for(i=0; i<times; i++) {
                for(j=0; j<para->n_ev; j++) {
                    p_ev->tp = para->ev[j];
                    p_ev->time = p_ch->time+i*para->evCalcCnt*step_ms;
                    dsp_ev_calc(p_ev->tp, p_raw->data+i*para->evCalcCnt, para->evCalcCnt, para->smpFreq, &p_ev->data);
                    p_ev++;
                }
            }
            
            p_ch->evlen = times*para->n_ev*sizeof(ev_data_t);
           memcpy((U8*)p_ch->data+p_ch->wavlen, ev_tmp, p_ch->evlen);  //在wav数据后面追加ev数据
        }
        
        tlen = p_ch->wavlen+p_ch->evlen;
    }
    else { //如果计算ev的数据个数比较多，大于一次采集的数据, 则进行缓存后再计算
        
        if(p_ch->wavlen>0 && p_ch->time>0) {    //已经有缓冲数据
            if(p_ch->wavlen+wavlen>=ev_calc_wavlen){
                int xlen=ev_calc_wavlen-p_ch->wavlen;
                memcpy((U8*)p_ch->data+p_ch->wavlen, p_raw->data, xlen);
                p_ch->wavlen += xlen;
            }
            else {
                memcpy((U8*)p_ch->data+p_ch->wavlen, p_raw->data, wavlen);
                p_ch->wavlen += wavlen;
            }
        }
        else { //空数据，需要填充头部
            p_ch->ch = ch;
            p_ch->time = p_raw->time;
            p_ch->wavlen = 0;
            
            memcpy((U8*)p_ch->data, p_raw->data, wavlen);
            p_ch->wavlen += wavlen;
        }
        
        //数据缓冲完成则进行计算
        if(p_ch->wavlen>=ev_calc_wavlen) {
            for(j=0; j<para->n_ev; j++) {
                p_ev->tp = para->ev[j];
                p_ev->time = p_ch->time;
                dsp_ev_calc(p_ev->tp, p_ch->data, para->evCalcCnt, para->smpFreq, &p_ev->data);
                p_ev++;
            }
            
            p_ch->evlen = para->n_ev*sizeof(ev_data_t);
            memcpy((U8*)p_ch->data+p_ch->wavlen, ev_tmp, p_ch->evlen);
            
            tlen = p_ch->wavlen+p_ch->evlen;
        }
    }
    
#if 1
    if(tlen>0) {
        tlen += sizeof(ch_data_t);
        r = list_append(lsend, p_ch, tlen);
        if(r==0) {
            task_trig(TASK_COMM_SEND, EVT_DATA);
            memset(p_ch, 0, sizeof(ch_data_t));
        }
    }
    

    //周期采样时，达到设定的长度则停止采样
    if(smp->smp_mode==0) {
        if(tb->rlen[ch]>=smplen) {
            api_cap_stop(ch);
            //list_clear(l);
        }
        else {
            tb->rlen[ch] += wavlen;
        }
    }
#endif
    
    return r;
}


static int ch_data_proc(void)
{
    int ch,r=0;
    node_t nd;
    handle_t *raw=taskBuffer.raw;
    
    for(ch=0; ch<CH_MAX; ch++) {
        r = list_take(raw[ch], &nd, 0);      
        if(r==0) {
            data_proc_fn(raw[ch], ch, nd.buf, nd.dlen);
            list_back(raw[ch], &nd);
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


