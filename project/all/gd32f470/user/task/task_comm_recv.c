#include "net.h"
#include "log.h"
#include "dal.h"
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

static int comm_init(void);



static void recv_init()
{
    recvBuf.log.blen = 2000;
    recvBuf.log.buf = eMalloc(recvBuf.log.blen);
    recvBuf.log.dlen = 0;
    recvBuf.log.busying = 0;
    
    recvBuf.net.blen = 2000;
    recvBuf.net.buf = eMalloc(recvBuf.log.blen);
    recvBuf.net.dlen = 0;
    recvBuf.net.busying = 0;
    
    recvBuf.rs485.blen = 2000;
    recvBuf.rs485.buf = eMalloc(recvBuf.log.blen);
    recvBuf.rs485.dlen = 0;
    recvBuf.rs485.busying = 0;
}


static void comm_tmr_callback(void *arg)
{
    task_trig(TASK_COMM_RECV, EVT_TIMER);
}

static int buf_data_proc(buf_t *pb, void *data, int len, int flag, U8 evt)
{
    int r=0;
    
    if(!pb->busying && pb->buf && pb->dlen+len<pb->blen-1) {
        memcpy(pb->buf+pb->dlen, data, len);
        pb->dlen = +len;
        pb->buf[pb->dlen] = 0;
    }
    
    if(EVT_NET || flag>0) {
        r = task_trig(TASK_COMM_RECV, evt);
    }
    
    return r;
}

static int rs485_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len, int flag)
{
    return buf_data_proc(&recvBuf.rs485, data, len, flag, EVT_485);
}
static int net_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len, int flag)
{
    return buf_data_proc(&recvBuf.net, data, len, flag, EVT_NET);
}
static int log_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len, int flag)
{
    return buf_data_proc(&recvBuf.log, data, len, flag, EVT_LOG);
}

////////////////////////////////////////////////////////
#define POINTS(us,sfreq)    ((U64)us*sfreq/1000000)

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


#include "ecxxx.h"
static int hw_init(void)
{
    int r=0;
    dac_param_t dp;

    mem_init();
    log_init(log_recv_callback);
    upgrade_proc(NULL, 0);
    
    fs_init();
    json_init();
    paras_load();
    
    rtc2_init();
    
    buf_init();
    recv_init();
    
    at_init();
    
#if 0
    extern void cota_main(void *arg);
    hal_at_init();
    hal_at_power(1);
    hal_at_boot();
    start_task_simp(cota_main, 4096, NULL, NULL);
    while(1) osDelay(1000);
#endif
    comm_init();

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
    
    start_task(TASK_DATA_CAP,   osPriorityISR,    TASK_STACK_SIZE,  task_data_cap_fn,   5, NULL, 1);
    start_task(TASK_SEND,       osPriorityNormal, TASK_STACK_SIZE,  task_send_fn,       5, NULL, 1);
    start_task(TASK_DATA_PROC,  osPriorityNormal, TASK_STACK_SIZE,  task_data_proc_fn,  10, NULL, 1);
    
    api_polling_start();
}

static void start_cap(void)
{
#ifdef CAP_AUTO
    if(paras_get_mode()!=MODE_CALI) {
        api_cap_start_all();
    }
#endif
}


static int comm_init(void)
{
    rs485_cfg_t rc;
    comm_para_t comm_p;
    
    comm_p.rlen = 0;
    comm_p.tlen = 0;
    comm_p.para = NULL;
    
    comm_p.port = PORT_LOG;
    tasksHandle.hlog = comm_open(&comm_p);
    
    rc.port = RS485_PORT;
    rc.baudrate = 115200;
    rc.callback = rs485_recv_callback;
    
    comm_p.port = PORT_LOG;
    comm_p.para = &rc;
    tasksHandle.h485 = comm_open(&comm_p);
    
    return 0;
}



int api_comm_connect(U8 port)
{
    int r=0;
    comm_para_t comm_p;
    conn_para_t conn_p;

    comm_p.port = port;
    comm_p.rlen = 0;
    comm_p.tlen = 0;
    comm_p.para = NULL;
    if(!tasksHandle.hnet) {
        if(port==PORT_NET) {
            conn_p.para = &allPara.usr.net;
            conn_p.callback = net_recv_callback;
            conn_p.proto = PROTO_MQTT;
            comm_p.para  = &conn_p;
            
            hal_at_power(1);            //4g模组上电
        }
        
        tasksHandle.hnet = comm_open(&comm_p);
        if(tasksHandle.hnet) {
            LOGD("___ comm_open net ok!\n");
            r = 1;
        }
    }
    
    return r;
}

int api_comm_disconnect(void)
{
    if(tasksHandle.hnet) {
        
        api_polling_conn_wait();
        
        comm_close(tasksHandle.hnet);
        tasksHandle.hnet = NULL;
        
        hal_at_power(0);            //关4g模组
    }
    
    return 0;
}

int api_comm_is_connected(void)
{
    if(tasksHandle.hnet) {
        return 1;
    }
    
    return 0;
}


int api_comm_send_para(void)
{
    int r;
    mqtt_pub_para_t pub_para={DATA_SETT};
    
    return comm_send_data(tasksHandle.hnet, &pub_para, TYPE_PARA, 1, &allPara, sizeof(allPara));
}

int api_comm_send_data(U8 type, U8 nAck, void *data, int len)
{
    int r;
    handle_t h=tasksHandle.hnet;
    
    if(type==TYPE_CALI) {
        h = tasksHandle.hlog;
    }
    
    return r = comm_send_data(h, NULL, type, nAck, data, len);
}


//////////////////////////////////////////////////////////////////////////////////////////////

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
                case EVT_NET:
                {
                    buf_t *pb=&recvBuf.net;
                    
                    pb->busying = 1;
                    err = comm_recv_proc(tasksHandle.hnet, NULL, pb->buf, pb->dlen);
                    pb->dlen = 0;
                    pb->busying = 0;
                }
                break;
                
                case EVT_485:
                {
                    buf_t *pb=&recvBuf.rs485;
                    
                    pb->busying = 1;
                    err = comm_recv_proc(tasksHandle.h485, NULL, pb->buf, pb->dlen);
                    pb->dlen = 0;
                    pb->busying = 0;
                }
                break;
                
                case EVT_LOG:
                {
                    buf_t *pb=&recvBuf.log;
                    
                    pb->busying = 1;
                    r = comm_cmd_proc((char*)pb->buf, pb->dlen);
                    if(r) {
                        comm_recv_proc(tasksHandle.hlog, NULL, pb->buf, pb->dlen);
                    }
                    pb->dlen = 0;
                    pb->busying = 0;
                }
                break;
            }
        }
        
        //task_yield();
    }
}

#endif


