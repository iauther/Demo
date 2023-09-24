#include "task.h"
#include "list.h"
#include "paras.h"
#include "dal_adc.h"
#include "cfg.h"
#include "dsp.h"
#include "rtc.h"
#include "dal_delay.h"


//״̬������������״̬����������       �磺λ�á��ٶ�/���ٶȡ����������ܡ����ٶȡ��Ƕ�����ѹǿ���¶ȡ���������ܵ�
//����������������״̬�仯���̵����������磺�����������������ٶȸı����ȶ��ǹ�����������ʱ��仯�����Ӧ



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
        
        //����ݴ�����ͷ��ch_data_t
        p_ch->ch = ch;
        p_ch->time = p_raw->time;
        p_ch->wavlen = wavlen;
        memcpy(p_ch->data, p_raw->data, p_ch->wavlen);      //����wav����
        
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
           memcpy((U8*)p_ch->data+p_ch->wavlen, ev_tmp, p_ch->evlen);  //��wav���ݺ���׷��ev����
        }
        
        tlen = p_ch->wavlen+p_ch->evlen;
    }
    else { //�������ev�����ݸ����Ƚ϶࣬����һ�βɼ�������, ����л�����ټ���
        
        if(p_ch->wavlen>0 && p_ch->time>0) {    //�Ѿ��л�������
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
        else { //�����ݣ���Ҫ���ͷ��
            p_ch->ch = ch;
            p_ch->time = p_raw->time;
            p_ch->wavlen = 0;
            
            memcpy((U8*)p_ch->data, p_raw->data, wavlen);
            p_ch->wavlen += wavlen;
        }
        
        //���ݻ����������м���
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
    

    //���ڲ���ʱ���ﵽ�趨�ĳ�����ֹͣ����
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


