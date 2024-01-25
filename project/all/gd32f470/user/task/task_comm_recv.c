#include "net.h"
#include "log.h"
#include "dal.h"
#include "rs485.h"
#include "pkt.h"
#include "dac.h"
#include "fs.h"
#include "rtc.h"
#include "comm.h"
#include "cfg.h"
#include "mem.h"
#include "json.h"
#include "upgrade.h"
#include "dal_delay.h"
#include "dal_uart.h"
#include "paras.h"
#include "cmd.h"
#include "list.h"
#include "hal_at_impl.h"


#ifdef OS_KERNEL

#include "task.h"


typedef struct {
    buf_t log;
    buf_t net;
    buf_t rs485;
}recv_buf_t;

static recv_buf_t recvBuf;
tasks_handle_t tasksHandle={0};


static void recv_init()
{
    recvBuf.log.blen = 2000;
    recvBuf.log.buf = eMalloc(recvBuf.log.blen);
    recvBuf.log.dlen = 0;
    
    recvBuf.net.blen = 2000;
    recvBuf.net.buf = eMalloc(recvBuf.log.blen);
    recvBuf.net.dlen = 0;
    
    recvBuf.rs485.blen = 2000;
    recvBuf.rs485.buf = eMalloc(recvBuf.log.blen);
    recvBuf.rs485.dlen = 0;
}


static void comm_tmr_callback(void *arg)
{
    task_trig(TASK_COMM_RECV, EVT_TIMER);
}


static int rs485_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    buf_t *pb=&recvBuf.rs485;
    
    if(pb->buf && pb->dlen==0 && data && (len>0 && len<pb->blen-1)) {
        memcpy(pb->buf, data, len);
        pb->dlen = len;
        pb->buf[pb->dlen] = 0;
        
        task_trig(TASK_COMM_RECV, EVT_RS485);
    }
    
    return 0;
}
static int net_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    buf_t *pb=&recvBuf.net;
    
    if(pb->buf && pb->dlen==0 && data && (len>0 && len<pb->blen)) {
        memcpy(pb->buf, data, len);
        pb->dlen = len;
        pb->buf[pb->dlen] = 0;
        
        task_trig(TASK_COMM_RECV, EVT_COMM);
    }
    
    return 0;
}


static int log_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int  r=0;
    buf_t *pb=&recvBuf.log;
    
    //LOGD("&&: %s", data);
    if(pb->buf && pb->dlen==0 && data && (len>0 && len<pb->blen-1)) {
        memcpy(pb->buf, data, len);
        pb->dlen = len;
        pb->buf[pb->dlen] = 0;
        
        task_trig(TASK_COMM_RECV, EVT_LOG);
    }
    
    return r;
}

////////////////////////////////////////////////////////
#define POINTS(us,sfreq)    ((U64)us*sfreq/1000000)

static int max_data_len=0;
static void buf_init(void)
{
    int i;
    U8 *pbuf;
    list_cfg_t lc;
    rbuf_cfg_t rc;
    int cnt1,cnt2,cnt3;
    int len1,len2,len3,len4,maxlen;
    ch_para_t *para;
    task_buf_t *tb=&taskBuffer;
    
    lc.max = 10;
    lc.mode = LIST_FULL_FIFO;
    lc.log = 1;
    for(i=0; i<CH_MAX; i++) {
        para = paras_get_ch_para(i);
        
        if(tb->var[i].raw) {
            list_free(tb->var[i].raw);
            tb->var[i].raw=NULL;
        }
        tb->var[i].raw = list_init(&lc);
        
        cnt1 = POINTS(para->trigTime.preTime,para->smpFreq);
        tb->var[i].thr.pre.bcnt = cnt1;
        tb->var[i].thr.pre.dcnt = 0;
        tb->var[i].thr.pre.data = eMalloc(tb->var[i].thr.pre.bcnt*4);
        
        cnt2 = POINTS(para->trigTime.MDT,para->smpFreq);
        tb->var[i].thr.bdy.bcnt = cnt2;
        tb->var[i].thr.bdy.dcnt = 0;
        tb->var[i].thr.bdy.data = eMalloc(tb->var[i].thr.bdy.bcnt*4);
        
        cnt3 = POINTS(para->trigTime.postTime,para->smpFreq);
        tb->var[i].thr.post.bcnt = cnt3;
        tb->var[i].thr.post.dcnt = 0;
        tb->var[i].thr.post.data = eMalloc(tb->var[i].thr.post.bcnt*4);
        
        tb->var[i].thr.idx = -1;
        tb->var[i].thr.cur.pdt = -1;
        tb->var[i].thr.cur.hdt = -1;
        tb->var[i].thr.cur.hlt = -1;
        tb->var[i].thr.cur.mdt = -1;
        
        tb->var[i].thr.set.pdt = POINTS(para->trigTime.PDT, para->smpFreq);
        tb->var[i].thr.set.hdt = POINTS(para->trigTime.HDT, para->smpFreq);
        tb->var[i].thr.set.hlt = POINTS(para->trigTime.HLT, para->smpFreq);
        tb->var[i].thr.set.mdt = POINTS(para->trigTime.MDT, para->smpFreq);
        
        tb->var[i].thr.vth = pow(10,para->trigThreshold/20-3);
        tb->var[i].thr.max = 0.0f;
        tb->var[i].thr.time = 0;
        tb->var[i].thr.trigged = 0;
        
        tb->var[i].rlen = 0;
        tb->var[i].slen = 0;
        tb->var[i].times = 0;
        
        int points = (para->smpFreq/1000)*SAMPLE_INT_INTERVAL;
        
        //为采集任务申请临时内存
        if(tb->var[i].cap.buf) {
            xFree(tb->var[i].cap.buf);
        }
        tb->var[i].cap.blen = points*sizeof(raw_t)+sizeof(raw_data_t)+32;
        tb->var[i].cap.buf = iMalloc(tb->var[i].cap.blen); 
        
        
        //为计算任务申请临时内存
        if(tb->var[i].prc.buf) {
            xFree(tb->var[i].prc.buf);
        }
        
        //申请ev内存
        int ev_grps = points/para->evCalcPoints+1;
        int ev_len = sizeof(ev_data_t)+(sizeof(ev_grp_t)+sizeof(ev_val_t)*para->n_ev)*ev_grps;
        
        int once_smp_len=(para->smpFreq/1000)*SAMPLE_INT_INTERVAL*sizeof(raw_t)+32;
        int once_ev_calc_data_len=para->evCalcPoints*sizeof(raw_t);
        
        //计算单次采集能满足ev计算时所需内存长度
        len1 = once_smp_len+ev_len+sizeof(ch_data_t)+32;
        
        //计算特征值计算时需要的内存长度,特别是特征值计算点数大于采集点数时
        len2 = once_ev_calc_data_len+ev_len+sizeof(ch_data_t)+32;
        
        //计算阈值触发时缓存设定要求所需内存长度, (1ms+smpPoints)*2
        len3 = (cnt1+cnt2+cnt3)*4+ev_len+32;
        
        maxlen = MAX3(len1,len2,len3);
        if(maxlen>max_data_len) {
            max_data_len = maxlen;
        }
        
        tb->var[i].prc.blen = maxlen;
        tb->var[i].prc.buf  = eCalloc(tb->var[i].prc.blen); 
        tb->var[i].prc.dlen = 0;
    }
    tb->file = list_init(&lc);
    
    //lc.max = 10;
    tb->send = list_init(&lc);
}
static int at_init(void)
{
    smp_para_t *smp=paras_get_smp();
    
    hal_at_init();
    if(smp->pwrmode==PWR_PERIOD_PWRDN) {
        hal_at_power(0);
    }
    else {
        hal_at_power(1);
    }
    
    return 0;
}


static int hw_init(void)
{
    int r=0;
    dac_param_t dp;
    rs485_cfg_t rc;

    mem_init();
    log_init(log_recv_callback);
    
    
    upgrade_check(NULL, 0);
    
    fs_init();
    json_init();
    paras_load();
    
    rtc2_init();
    at_init();
    
    rc.port = RS485_PORT;
    rc.baudrate = 115200;
    rc.callback = rs485_recv_callback;
    rs485_init(&rc);
    
    buf_init();
    recv_init();

    return r;
}



static void start_task(int taskid, osPriority_t prio, int stkSize, osThreadFunc_t fn, int msgCnt, void *arg, int runNow)
{
    task_attr_t attr={0};
    
    attr.func = fn,
    attr.arg = arg,
    attr.nMsg = msgCnt,
    attr.taskID = taskid,
    attr.priority = prio;//osPriorityNormal,
    attr.stkSize = stkSize,
    attr.runNow  = runNow,
    task_new(&attr);
}

static void start_tasks(void)
{
    #define TASK_STACK_SIZE   2048
    
    start_task(TASK_DATA_CAP,   osPriorityNormal, TASK_STACK_SIZE,  task_data_cap_fn,   5, NULL, 1);
    start_task(TASK_POLLING,    osPriorityNormal, TASK_STACK_SIZE,  task_polling_fn,    5, NULL, 1);
    start_task(TASK_SEND,       osPriorityNormal, TASK_STACK_SIZE*2,  task_send_fn,        5, NULL, 1);
    
    start_task(TASK_DATA_PROC,  osPriorityNormal, TASK_STACK_SIZE,  task_data_proc_fn,  10, NULL, 1);
}

static void start_cap(void)
{
#ifdef AUTO_CAP
    if(paras_get_mode()!=MODE_CALI) {
        api_cap_start_all();
    }
#endif
}


int api_comm_connect(U8 port)
{
    int r=0;
    comm_para_t comm_p;
    conn_para_t conn_p;

    comm_p.port = port;
    comm_p.rlen = 0;
    comm_p.tlen = max_data_len;
    comm_p.para = NULL;
    if(!tasksHandle.hcomm) {
        if(port==PORT_NET) {
            conn_p.para = &allPara.usr.net;
            conn_p.callback = net_recv_callback;
            conn_p.proto = PROTO_MQTT;
            comm_p.para = &conn_p;
            
            hal_at_power(1);            //4g模组上电
        }
        
        tasksHandle.hcomm = comm_open(&comm_p);
        if(tasksHandle.hcomm) {
            LOGD("___ comm_open ok!\n");
            r = 1;
        }
    }
    
    return r;
}

int api_comm_disconnect(void)
{
    if(tasksHandle.hcomm) {
        comm_close(tasksHandle.hcomm);
        tasksHandle.hcomm = NULL;
        
        hal_at_power(0);            //关4g模组
    }
    
    return 0;
}

int api_comm_is_connected(void)
{
    if(tasksHandle.hcomm) {
        return 1;
    }
    
    return 0;
}


int api_comm_send_para(void)
{
    int r;
    mqtt_pub_para_t pub_para={DATA_SETT};
    
    if(!tasksHandle.hcomm) {
        return -1;
    }
    
    r = comm_send_data(tasksHandle.hcomm, &pub_para, TYPE_PARA, 1, &allPara, sizeof(allPara));
    
    return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////

static void user_cmd_proc(cmd_t *cmd)
{
    int r;
    
    switch(cmd->ID) {
        case CMD_USER:
        {
            //cmd->str
            
        }
        break;
        
        case CMD_DFLT:
        {
            paras_factory();
            dal_reboot();
        }
        break;
        
        case CMD_BOOT:
        {
            dal_reboot();
        }
        break;
        
        case CMD_CAP:
        {
            U8 flag=0;
            sscanf(cmd->str, "%hhu", &flag);
            
            paras_set_mode(MODE_NORM);
            if(flag>0) {
                api_cap_start_all();
            }
            else {
                api_cap_stop_all();
            }
        }
        break;
        
        case CMD_CALI:
        {
            /*
                u8	                                %d
                s8	                                %d
                u16	                                %d or %hu
                s16	                                %d or %hd
                u32	                                %u
                s32	                                %d
                u64	                                %llu
                s64	                                %lld
                int	                                %d
                unsigned int	                    %u
                short int	                        %d or %hd
                long	                            %ld
                unsigned long	                    %lu
                long long	                        %lld
                unsigned long long	                %llu
                char	                            %c
                char*	                            %s
                bool            	                %d
                unsigned int/int        	        %0x
                unsigned long/long                  %0lx
                long long/unsigned long long        %0llx
                unsigned int/int                    %0o
                unsigned long/long          	    %0lo
                long long/unsigned long long        %0llo
                float	                            %f
                double	                            %f or %lf
                科学技术类型(必须转化为double类型)	%e
                限制输出字段宽度	                    %x.yf (x:整数长度,y:小数点长度)
            */
            cali_sig_t sig;
            r = sscanf(cmd->str, "%hhu,%u,%f,%hhu,%f,%hhu,%hhu", &sig.ch, &sig.freq, &sig.bias, &sig.lv, &sig.volt, &sig.max, &sig.seq);
            if(r!=7) {
                LOGE("___ cali para is wrong!\n");
                return;
            }
            
            task_post(TASK_POLLING, NULL, EVT_CALI, 0, &sig, sizeof(sig));
        }
        break;
        
        case CMD_DAC:
        {
            dac_param_t param;
            ch_para_t *para = paras_get_ch_para(CH_0);
            
            param.enable = 0;
            param.fdiv = 1;
            
            sscanf(cmd->str, "%d,%d", &param.enable, &param.fdiv);
            LOGD("___ config dac, en: %d, fdiv: %d\n", param.enable, param.fdiv);
            
            int points = (para->smpFreq/1000)*SAMPLE_INT_INTERVAL;
            param.freq = para->smpFreq;
            param.buf.blen = points*sizeof(U16);
            param.buf.buf  = eMalloc(param.buf.blen);
            param.buf.dlen = 0;
            dac_set(&param);
        }
        break;
        
        case CMD_PWR:
        {
            int r;
            U32 countdown=1;
            
            sscanf(cmd->str, "%d", &countdown);
            
            LOGD("___ powerdown now, restart %d seconds later!\n", countdown);
            r = rtc2_set_countdown(countdown);
            LOGD("___ rtc2_set_countdown result: %d\n", r);
        }
        break;
    }
    
}



void task_comm_recv_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    
    hw_init();
    
    LOGD("_____ task comm recv running\n");
    start_tasks();
    start_cap();
    
    while(1) {
        r = task_recv(TASK_COMM_RECV, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                case EVT_COMM:
                {
                    buf_t *pb=&recvBuf.net;
                    
                    err = comm_recv_proc(tasksHandle.hcomm, NULL, pb->buf, pb->dlen);
                    pb->dlen = 0;
                }
                break;
                
                case EVT_RS485:
                {
                    buf_t *pb=&recvBuf.rs485;
                    
                    err = comm_recv_proc(tasksHandle.hcomm, NULL, pb->buf, pb->dlen);
                    pb->dlen = 0;
                }
                break;
                
                case EVT_LOG:
                {
                    cmd_t cmd;
                    buf_t *pb=&recvBuf.log;
                    
                    r = cmd_get((char*)recvBuf.log.buf, &cmd);
                    if(r==0) {
                        user_cmd_proc(&cmd);
                    }
                    pb->dlen = 0;
                }
                break;
            }
        }
        
        //task_yield();
    }
}

#endif


