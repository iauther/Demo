#include "net.h"
#include "log.h"
#include "dal.h"
#include "rs485.h"
#include "pkt.h"
#include "dac.h"
#include "fs.h"
#include "rtc.h"
#include "comm.h"
#include "mem.h"
#include "json.h"
#include "lock.h"
#include "dal_delay.h"
#include "paras.h"
#include "cmd.h"
#include "list.h"


#ifdef OS_KERNEL

#include "task.h"


tasks_handle_t tasksHandle={0};

static void recv_list_init(void)
{
    list_cfg_t lc;
    tasks_handle_t *th=&tasksHandle;
    
    lc.max = 10;
    lc.mode = MODE_FIFO;
    th->rcv = list_init(&lc);
}

static void comm_tmr_callback(void *arg)
{
    task_post(TASK_COMM_RECV, NULL, EVT_TIMER, 0, NULL, 0);
}


static int rs485_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    task_post(TASK_COMM_RECV, NULL, EVT_RS485, 0, NULL, 0);
    
    return 0;
}
static int net_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int r = list_append(tasksHandle.rcv, 0, data, len);
    if(r==0) {
        task_trig(TASK_COMM_RECV, EVT_COMM);
    }
    
    return 0;
}


static int log_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int  r=0;
    cmd_t cmd;
    
    //LOGD("&&: %s", data);
    r = cmd_get((char*)data, &cmd);
    if(r==0) {
        task_post(TASK_COMM_RECV, NULL, EVT_LOG, 0, &cmd, sizeof(cmd));
    }
    else {
        r = list_append(tasksHandle.rcv, 0, data, len);
        if(r==0) {
            task_trig(TASK_COMM_RECV, EVT_COMM);
        }
    }
    
    return r;
}

////////////////////////////////////////////////////////

static int hw_init(void)
{
    int r=0;
    U8 port;
    
    mem_init();
    log_init(log_recv_callback);
    rtc2_init();
    
    fs_init();
    json_init();
    paras_load();
    dac_init();
    recv_list_init();
    
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
    start_task(TASK_DATA_CAP,   osPriorityNormal, 2048,  task_data_cap_fn,   5, NULL, 1);
    start_task(TASK_POLLING,    osPriorityNormal, 2048,  task_polling_fn,    5, NULL, 1);
    start_task(TASK_NVM,        osPriorityNormal, 2048,  task_nvm_fn,        5, NULL, 1);
    
    start_task_simp(task_data_proc_fn, 2048, NULL, NULL);
    //start_task_simp(task_comm_send_fn, 2048, NULL, NULL);
    
}



int api_comm_connect(U8 port)
{
    int r=0;
    
    if(port==PORT_NET) {
        net_cfg_t cfg;
        conn_para_t cp;
        
        if(!tasksHandle.hcomm) {
            tasksHandle.hcomm = comm_init(port, &cfg);
        }
        
        if(tasksHandle.hcomm && !tasksHandle.hconn) {
            cp.para = &allPara.usr.net;
            cp.callback = net_recv_callback;
            cp.proto = PROTO_MQTT;
            tasksHandle.hconn = comm_open(tasksHandle.hcomm, &cp);
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
            tasksHandle.hcomm = comm_init(port, NULL);
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
    U8 to=DATO_USR;
    
    if(!tasksHandle.hcomm || !tasksHandle.hconn) {
        return -1;
    }
    
    r = comm_send_data(tasksHandle.hconn, &to, TYPE_PARA, 1, &allPara, sizeof(allPara));
    
    return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////

static void user_cmd_proc(cmd_t *cmd)
{
    switch(cmd->ID) {
        case CMD_USER:
        {
            //cmd->str
            
        }
        break;
        
        case CMD_CALI:
        {
            cali_sig_t sig={
                .tms = 1,
                .ch = CH_0,
                .rms = 15,
                .freq = 40000,
                .bias = 0,
            };
            
            sscanf(cmd->str, "%hu,%hu,%f,%d,%f", &sig.tms, &sig.ch, &sig.rms, &sig.freq, &sig.bias);
            LOGD("___ calibration para, tms: %hu, ch: %hu, rms: %fmv, freq: %u, bias: %f\n", sig.tms, sig.ch, sig.rms, sig.freq, sig.bias);
            
            paras_set_cali_sig(sig.ch, &sig);
            api_cap_start(sig.ch);
        }
        break;
        
        case CMD_DAC:
        {
            dac_param_t param;
            
            param.enable = 0;
            param.fdiv = 1;
            
            sscanf(cmd->str, "%d,%d", &param.enable, &param.fdiv);
            LOGD("___ config dac, en: %d, fdiv: %d\n", param.enable, param.fdiv);
            
            param.freq = 1000000;
            param.points = paras_get_smp_cnt(CH_0);
            
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
                    list_node_t *lnode=NULL;
                    handle_t list=tasksHandle.rcv;
                    
                    if(list_take_node(list, &lnode, 0)==0) {
                        err = comm_recv_proc(tasksHandle.hconn, NULL, lnode->data.buf, lnode->data.dlen);
                        list_back_node(list, lnode);
                    }
                }
                break;
                
                case EVT_RS485:
                {
                    
                }
                break;
                
                case EVT_LOG:
                {
                    cmd_t *cmd=(cmd_t*)e.data;
                    user_cmd_proc(cmd);
                }
                break;
            }
        }
        
        task_yield();
    }
}

#endif


