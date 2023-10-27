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


//����0ʱ������������󴫣���������ֵ���㡢���̡��ϴ��ȴ���
static int find_amp_max_over_threshold(U8 ch, U64 time, F32 *data, int cnt, ch_para_t *para)
{
    int i,j;
    U64 t;
    static F32 max=-1.0f;
    ch_var_t  *cv=&taskBuffer.var[ch];
    buf_t     *buf=&cv->prc;          //block buffer
    handle_t  rbuf=cv->rb;            //ring  buffer
    F32 x=fabs(data[0]);
    int one_ms_len=para->smpFreq/1000;
    
    for(i=1; i<cnt; i++) {
        if(fabs(data[i])>x) {
            x = fabs(data[i]);
            j = i;
        }
    }
    
    if(x>para->ampThreshold) {
        
        if(x>max) {
            max = x;
            cv->ts[0] = rtc2_get_timestamp_ms();
            
            //��������
            if(j<one_ms_len) {      //����1ms����
                
                //here can use block buf or ring buf to cache the data
                //memcpy();
                //rbuf_write(rbuf, data[j]);
            }
            else {
                //memcpy(buf->buf, data+j, );
            }
        }
        else {  //����ʱ�����messDurationʱ����С��trigDelay�������������
            
            t = rtc2_get_timestamp_ms();
            
            //��ǰʱ����뷢�ֳ���ֵ��ʱ��
            if(t-cv->ts[0]>=para->messDuration) {
                
                if(cv->ts[1]>0 && (t-cv->ts[1]<para->trigDelay)) {
                    return -1;
                }
                
                cv->ts[1] = t;
                return 0;
            }
        }
    }
    
    return -1;
}



static int data_proc_fn(handle_t l, U8 ch, F32 *data, int len)
{
    int i,j,r;
    int tlen=0;
    U64 time;
    U32 step_ms;
    task_buf_t *tb=&taskBuffer;
    smp_para_t *smp=paras_get_smp();
    ch_para_t *para=paras_get_ch_para(ch);
    int smp_len=para->smpPoints*sizeof(raw_t);
    int ev_calc_len=para->evCalcCnt*sizeof(raw_t);
    int t_skp_len=para->smpInterval*(para->smpFreq*sizeof(raw_t)/1000);
    raw_data_t *p_raw=(raw_data_t*)data;
    ch_var_t *cv=&tb->var[ch];
    ch_data_t  *p_ch=(ch_data_t*)cv->prc.buf;
    ev_data_t *p_ev=(ev_data_t*)tb->var[ch].ev.buf;
    ev_data_t *p_ev2=NULL;
    int data_len=len-sizeof(raw_data_t);
    
    raw_t *real_data=p_raw->data;
    int    real_len=data_len;
    
    if(para->smpMode==SMP_TRIG_MODE) {
        buf_t *buf=&cv->prc;
        
        r = find_amp_max_over_threshold(ch, p_raw->time, p_raw->data, data_len/sizeof(raw_t), para);
        if(r) {
            return r;
        }
        
        real_data=(raw_t*)buf->buf;
        real_len=buf->dlen;
    }
    
    
    step_ms = 1000/para->smpFreq;
    if(real_len>=ev_calc_len) {
        
        //����ݴ�����ͷ��ch_data_t
        p_ch->ch = ch;
        p_ch->time = p_raw->time;
        p_ch->wavlen = real_len;
        memcpy(p_ch->data, real_data, p_ch->wavlen);      //����wav����
        
        if(para->n_ev>0) {
            ev_val_t val;
            ev_grp_t *grp=p_ev->grp;
            p_ev->grps = real_len/ev_calc_len;
            
            for(i=0; i<p_ev->grps; i++) {
                
                grp->time = p_ch->time+i*para->evCalcCnt*step_ms;
                grp->cnt = para->n_ev;
                
                for(j=0; j<grp->cnt; j++) {
                    val.tp = para->ev[j];
                    dsp_ev_calc(val.tp, real_data+i*para->evCalcCnt, para->evCalcCnt, para->smpFreq, &val.data);
                    
                    grp->val[j] = val;
                }
                grp = (ev_grp_t*)((U8*)grp + sizeof(ev_grp_t) + sizeof(ev_val_t)*grp->cnt);
            }
            
            p_ch->evlen = sizeof(ev_data_t)+p_ev->grps*(sizeof(ev_grp_t)+para->n_ev*sizeof(ev_val_t));
            
            p_ev2 = (ev_data_t*)((U8*)p_ch->data+p_ch->wavlen);
            memcpy(p_ev2, p_ev, p_ch->evlen);  //��wav���ݺ���׷��ev_data_t����
        }
        
        tlen = p_ch->wavlen+p_ch->evlen;
    }
    else { //�������ev�����ݸ����Ƚ϶࣬����һ�βɼ�������, ����л�����ټ���
        
        if(p_ch->wavlen>0 && p_ch->time>0) {    //�Ѿ��л�������
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
        else { //�����ݣ���Ҫ���ͷ��
            p_ch->ch = ch;
            p_ch->time = p_raw->time;
            p_ch->wavlen = 0;
            
            memcpy((U8*)p_ch->data, real_data, real_len);
            p_ch->wavlen += real_len;
        }
        
        //���ݻ����������м���
        if(p_ch->wavlen>=ev_calc_len) {
            
            ev_grp_t *grp=p_ev->grp;
            p_ev->grps = 1;
            
            grp->time = p_ch->time;
            grp->cnt = para->n_ev;
            
            ev_val_t *val=grp->val;
            for(j=0; j<para->n_ev; j++) {
                val[j].tp = para->ev[j];
                dsp_ev_calc(val[j].tp, p_ch->data, para->evCalcCnt, para->smpFreq, &val[j].data);
            }
            
            p_ch->evlen = sizeof(ev_data_t)+1*(sizeof(ev_grp_t)+para->n_ev*sizeof(ev_val_t));
            memcpy((U8*)p_ch->data+p_ch->wavlen, p_ev, p_ch->evlen);  //��wav���ݺ���׷��ev����
            
            tlen = p_ch->wavlen+p_ch->evlen;
        }
    }
    
      
    if(tlen>0) {
        
        //LOGD("_______________ RMS: %f\n", ((ev_data_t*)((U8*)(p_ch->data)+p_ch->wavlen))->grp[0].val[0].data);
        tlen += sizeof(ch_data_t);
        r = api_nvm_send(p_ch, tlen);
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


