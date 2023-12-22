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


static void recv_buf_init()
{
    recvBuf.log.blen = 2000;
    recvBuf.log.buf = eCalloc(recvBuf.log.blen);
    recvBuf.log.dlen = 0;
    
    recvBuf.net.blen = 2000;
    recvBuf.net.buf = eCalloc(recvBuf.log.blen);
    recvBuf.net.dlen = 0;
    
    recvBuf.rs485.blen = 2000;
    recvBuf.rs485.buf = eCalloc(recvBuf.log.blen);
    recvBuf.rs485.dlen = 0;
}


static void comm_tmr_callback(void *arg)
{
    task_post(TASK_COMM_RECV, NULL, EVT_TIMER, 0, NULL, 0);
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
static int max_data_len=0;
static void buf_init(void)
{
    int i;
    list_cfg_t lc;
    int len1,len2,len3,maxlen;
    ch_para_t *para;
    task_buf_t *tb=&taskBuffer;
    
    lc.max = 10;
    lc.mode = MODE_FULL_FIFO;
    lc.log = 1;
    for(i=0; i<CH_MAX; i++) {
        
        if(tb->var[i].raw) {
            list_free(tb->var[i].raw);
            tb->var[i].raw=NULL;
        }
        tb->var[i].raw = list_init(&lc);
        
        tb->var[i].rlen = 0;
        tb->var[i].slen = 0;
        tb->var[i].times = 0;
        tb->var[i].ts[0] = 0;
        tb->var[i].ts[1] = 0;
        
        para = paras_get_ch_para(i);
        int points = (para->smpFreq/1000)*SAMPLE_INT_INTERVAL;
        
        //为采集任务申请临时内存
        if(tb->var[i].cap.buf) {
            xFree(tb->var[i].cap.buf);
        }
        tb->var[i].cap.blen = points*sizeof(raw_t)+sizeof(raw_data_t)+32;
        tb->var[i].cap.buf = eCalloc(tb->var[i].cap.blen); 
        
        
        //为计算任务申请临时内存
        if(tb->var[i].prc.buf) {
            xFree(tb->var[i].prc.buf);
        }
        
        //申请ev内存
        
        int ev_grps = points/para->evCalcCnt+1;
        int ev_len = sizeof(ev_data_t)+(sizeof(ev_grp_t)+sizeof(ev_val_t)*para->n_ev)*ev_grps;
        tb->var[i].ev.blen = ev_len;
        tb->var[i].ev.buf  = eCalloc(tb->var[i].ev.blen); 
        tb->var[i].ev.dlen = 0;
        
        
        int once_smp_len=(para->smpFreq/1000)*SAMPLE_INT_INTERVAL*sizeof(raw_t)+32;
        int once_ev_calc_data_len=para->evCalcCnt*sizeof(raw_t);
        
        //计算单次采集能满足ev计算时所需内存长度
        len1 = once_smp_len+ev_len+sizeof(ch_data_t)+32;
        
        //计算特征值计算时需要的内存长度
        len2 = once_ev_calc_data_len+ev_len+sizeof(ch_data_t)+32;
        
        //计算阈值触发时缓存设定要求所需内存长度, (1ms+smpPoints)*2
        len3 = (para->smpPoints+para->smpFreq/1000)*2*sizeof(raw_t)+32;
        
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


static int hw_init(void)
{
    int r=0;
    dac_param_t dp;
    rs485_cfg_t rc;

    mem_init();
    log_init(log_recv_callback);
    
    rtc2_init();
    upgrade_check(NULL, 0);
    
    hal_at_init();
    

#if 0
    extern void cota_demo_fn(void *arg);
    extern void mota_demo_fn(void *arg);
    extern void fota_demo_fn(void *arg);
    start_task_simp(fota_demo_fn, 4096, NULL, NULL);
    while(1) osDelay(1000);
#endif
    
    
    fs_init();
    json_init();
    paras_load();
    
    rc.port = RS485_PORT;
    rc.baudrate = 115200;
    rc.callback = rs485_recv_callback;
    //rs485_init(&rc);
    
    buf_init();
    recv_buf_init();
    

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
    start_task(TASK_NVM,        osPriorityNormal, TASK_STACK_SIZE,  task_nvm_fn,        5, NULL, 1);
    
    start_task_simp(task_data_proc_fn, TASK_STACK_SIZE, NULL, NULL);
}



int api_comm_connect(U8 port)
{
    int r=0;
    comm_init_para_t comm_p;
    
    comm_p.rlen = 0;
    comm_p.tlen = max_data_len;
    comm_p.para = NULL;
    if(port==PORT_NET) {
        net_cfg_t cfg;
        
        comm_p.para = &cfg;
        if(!tasksHandle.hcomm) {
            tasksHandle.hcomm = comm_init(port, &comm_p);
        }
        
        if(tasksHandle.hcomm && !tasksHandle.hconn) {
            conn_para_t conn_p;
            
            conn_p.para = &allPara.usr.net;
            conn_p.callback = net_recv_callback;
            conn_p.proto = PROTO_MQTT;
            tasksHandle.hconn = comm_open(tasksHandle.hcomm, &conn_p);
            if(tasksHandle.hconn) {
                LOGD("___ comm_open ok!\n");
            }
        }
        else {
            r = 1;
        }
    }
    else {
        if(!tasksHandle.hcomm) {
            tasksHandle.hcomm = comm_init(port, &comm_p);
        }
        
        if(tasksHandle.hcomm && !tasksHandle.hconn) {
            tasksHandle.hconn = comm_open(tasksHandle.hcomm, NULL);
            if(tasksHandle.hconn) {
                LOGD("___ comm_open ok!\n");
            }
        }
        else {
            r = 1;
        }
    }
    
    return r;
}

int api_comm_disconnect(void)
{
    if(tasksHandle.hconn) {
        comm_close(tasksHandle.hconn);
        tasksHandle.hconn = NULL;
    }
    
    hal_at_power(0);            //关4g模组
    
    return 0;
}

int api_comm_is_connected(void)
{
    if(tasksHandle.hcomm && tasksHandle.hconn) {
        return 1;
    }
    
    return 0;
}


int api_comm_send_para(void)
{
    int r;
    mqtt_pub_para_t pub_para={DATA_SETT};
    
    if(!tasksHandle.hcomm || !tasksHandle.hconn) {
        return -1;
    }
    
    r = comm_send_data(tasksHandle.hconn, &pub_para, TYPE_PARA, 1, &allPara, sizeof(allPara));
    
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
            if(r==7) {
                task_post(TASK_POLLING, NULL, EVT_CALI, 0, &sig, sizeof(sig));
            }
            else {
                LOGE("___ cali para is wrong!\n");
            }  
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
            param.buf.buf  = eCalloc(param.buf.blen);
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
    
    while(1) {
        r = task_recv(TASK_COMM_RECV, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                case EVT_COMM:
                {
                    buf_t *pb=&recvBuf.net;
                    
                    err = comm_recv_proc(tasksHandle.hconn, NULL, pb->buf, pb->dlen);
                    pb->dlen = 0;
                }
                break;
                
                case EVT_RS485:
                {
                    buf_t *pb=&recvBuf.rs485;
                    
                    err = comm_recv_proc(tasksHandle.hconn, NULL, pb->buf, pb->dlen);
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


