#include "task.h"
#include "date.h"

#define TIMER_MS            100
#define TIMEOUT             500
#define RETRIES             (TIMEOUT/TIMER_MS)

static BYTE dev_rx_buf[1000];
static int dev_exit_flag=0;
static int paras_tx_flag=0;
static UINT_PTR timerId;
static HANDLE devRxThreadHandle, devThreadHandle;
extern paras_t DEFAULT_PARAS;

static void com_init(void)
{
    pkt_cfg_t cfg;

    curParas = DEFAULT_PARAS;
    get_date_string(DEFAULT_PARAS.fwInfo.bldtime, curParas.fwInfo.bldtime);

    cfg.retries = RETRIES;
    pkt_init(&cfg);
}

static U8 cmd_proc(void* data)
{
    cmd_t* c = (cmd_t*)data;
    //n950_send_cmd(c->cmd, c->para);

    return ERROR_NONE;
}


static U8 upgrade_proc(void* data)
{
    int r = 0;
    static U16 upg_pid = 0;
    upgrade_pkt_t* upg = (upgrade_pkt_t*)data;

    if (upg->pid == 0) {
        upg_pid = 0;
    }

    if (upg->pid != upg_pid) {
        return ERROR_FW_PKT_ID;
    }

    if (upg->pid == upg->pkts - 1) {
        //upgrade finished
    }
    else {
        upg_pid++;
    }

    return r;
}

const char* typeString[TYPE_MAX] = {
    "TYPE_CMD",
    "TYPE_STAT",
    "TYPE_ACK",
    "TYPE_SETT",
    "TYPE_PARAS",
    "TYPE_ERROR",
    "TYPE_UPGRADE",
};
static void err_print(U8 type, U8 err)
{
    switch (err) {
    case ERROR_NONE:
        LOG("___ pkt is ok\n");
        break;
    case ERROR_DEV_PUMP_RW:
        LOG("___ pump read/write error\n");
        break;
    case ERROR_DEV_MS4525_RW:
        LOG("___ ms4525 read/write error\n");
        break;
    case ERROR_DEV_E2PROM_RW:
        LOG("___ eeprom read/write error\n");
        break;
    case ERROR_PKT_TYPE:
        LOG("___ pkt type is wrong\n");
        break;
    case ERROR_PKT_MAGIC:
        LOG("___ pkt magic is wrong\n");
        break;
    case ERROR_PKT_LENGTH:
        LOG("___ pkt length is wrong\n");
        break;
    case ERROR_PKT_CHECKSUM:
        LOG("___ pkt checksum is wrong\n");
        break;
    case ERROR_DAT_TIMESET:
        LOG("___ para timeset is wrong\n");
        break;
    case ERROR_DAT_VACUUM:
        LOG("___ para vacuum is wrong\n");
        break;
    case ERROR_DAT_VOLUME:
        LOG("___ para volume is wrong\n");
        break;
    case ERROR_FW_INFO_VERSION:
        LOG("___ fw version is wrong\n");
        break;
    case ERROR_FW_INFO_BLDTIME:
        LOG("___ fw buildtime is wrong\n");
        break;
    case ERROR_FW_PKT_ID:
        LOG("___ fw pkt id is wrong\n");
        break;
    case ERROR_FW_PKT_LENGTH:
        LOG("___ fw pkt length is wrong\n");
        break;
    case ERROR_ACK_TIMEOUT:
        LOG("___ %s, ack is timeout\n", typeString[type]);
        break;
    default:
        break;
    }
}
static void pkt_print(pkt_hdr_t* p, U16 len)
{
    LOG("_____ pkt length:   %d\n", len);

    LOG("_____ pkt.magic: 0x%08x\n", p->magic);
    LOG("_____ pkt.type:  0x%02x\n", p->type);
    LOG("_____ pkt.flag:  0x%02x\n", p->flag);
    LOG("_____ pkt.dataLen: %d\n",   p->dataLen);

    if (p->dataLen>0) {
        switch (p->type) {
        case TYPE_CMD:
        {
            if (p->dataLen==sizeof(cmd_t)) {
                cmd_t* cmd = (cmd_t*)p->data;
                LOG("_____ cmd.cmd: 0x%08x, cmd.para: %d\n", cmd->cmd, cmd->para);
            }
        }
        break;
        case TYPE_ACK:
        {
            if (p->dataLen == sizeof(ack_t)) {
                ack_t* ack = (ack_t*)p->data;
                LOG("_____ ack.type: %d\n", ack->type);
            }
        }
        break;
        case TYPE_SETT:
        {
            if (p->dataLen == sizeof(setts_t)) {
                setts_t* setts = (setts_t*)p->data;
                LOG("_____ setts data\n");
            }
            else if (p->dataLen == sizeof(sett_t)) {
                sett_t* set = (sett_t*)p->data;
                LOG("_____ sett data\n");
            }
            else if (p->dataLen == sizeof(U8)) {
                U8* mode = (U8*)p->data;
                LOG("_____ mode: 0x%08x\n", *mode);
            }
        }
        break;
        case TYPE_PARAS:
        {
            if (p->dataLen == sizeof(paras_t)) {
                paras_t* paras = (paras_t*)p->data;
                LOG("_____ PARAS DATA\n");
            }
        }
        break;
        case TYPE_ERROR:
        {
            if (p->dataLen == sizeof(error_t)) {
                error_t* err = (error_t*)p->data;
                LOG("_____ err.error: %d\n", err->error);
            }
        }
        break;
        case TYPE_UPGRADE:
        {
            if (p->dataLen == sizeof(upgrade_pkt_t)) {
                upgrade_pkt_t* up = (upgrade_pkt_t*)p->data;
                LOG("_____ pkt.obj:  %d\n", up->obj);
                LOG("_____ pkt.pkts: %d\n", up->pkts);
                LOG("_____ pkt.pid:  %d\n", up->pid);
                LOG("_____ pkt.dataLen: %d\n", up->dataLen);
            }
        }
        break;
        case TYPE_LEAP:
        {
            if (p->dataLen == 0) {
                LOG("_____ pkt leap\n");
            }
        }
        break;
        }
    }
}


static U8 com_proc(pkt_hdr_t* p, U16 len)
{
    U8 err=0;

    if (p->askAck) {
        pkt_send_ack(p->type, 0);
    }

    pkt_print(p, len);
    err = pkt_hdr_check(p, len);
    if (err == ERROR_NONE) {
        switch (p->type) {
        case TYPE_CMD:
        {
            err = cmd_proc(p);
        }
        break;

        case TYPE_ACK:
        {
            ack_t* ack = (ack_t*)p->data;
            pkt_ack_update(ack->type);
        }
        break;

        case TYPE_SETT:
        {
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(sett_t)) {
                    sett_t* sett = (sett_t*)p->data;
                    curParas.setts.sett[sett->mode] = *sett;
                }
                else if (p->dataLen == sizeof(curParas.setts.mode)) {
                    U8* m = (U8*)p->data;
                    curParas.setts.mode = *m;
                }
                else {
                    memcpy(&curParas.setts.sett, p->data, sizeof(curParas.setts));
                }
            }
            else {
                pkt_send(TYPE_SETT, 0, &curParas.setts, sizeof(curParas.setts));
            }
        }
        break;

        case TYPE_PARAS:
        {
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(paras_t)) {
                    paras_t* paras = (paras_t*)p->data;
                    curParas = *paras;
                }
                else {
                    err = ERROR_PKT_LENGTH;
                }
            }
            else {
                pkt_send(TYPE_PARAS, 0, &curParas, sizeof(curParas));
                paras_tx_flag = 1;
            }
        }
        break;

        case TYPE_UPGRADE:
        {
            err = upgrade_proc(p->data);
        }
        break;

        case TYPE_LEAP:
        {

        }
        break;

        default:
        {
            err = ERROR_PKT_TYPE;
        }
        break;
        }
    }

    if (err) {
        err_print(p->type, err);
        pkt_send_err(p->type, err);
    }

    return err;
}

static int send_stat(U8 st)
{
    stat_t stat;
            
    stat.stat = st;
    stat.temp = 22.34F;
    stat.aPres = 93.6F;
    stat.dPres = 77.2F;
    stat.pump.speed = 500;
    stat.pump.current = 450.0F;
    stat.pump.temp = 44.9F;
    stat.pump.fault = 0;
    stat.pump.cmdAck = 1;
    pkt_send(TYPE_STAT, 0, &stat, sizeof(stat));
    
    return 0;
}


static void timer_proc(void)
{
    U8  i;
    int r;   

    if (!port_is_opened()) {
        return;
    }

    //if (!paras_tx_flag) {
        pkt_send(TYPE_PARAS, 1, &curParas, sizeof(curParas));
        paras_tx_flag = 1;
    //}
#if 0
    for (i = 0; i < TYPE_MAX; i++) {
        r = pkt_ack_timeout_check(i);
        if (r) {
            err_print(i, ERROR_ACK_TIMEOUT);
        }
    }
#endif

    send_stat(STAT_AUTO);
}



static DWORD WINAPI dev_rx_thread(LPVOID lpParam)
{
    int rlen;
    pkt_hdr_t* p = (pkt_hdr_t*)dev_rx_buf;

    while (1) {
        rlen = port_recv(p, sizeof(dev_rx_buf));
        if (rlen > 0) {
            com_proc(p, rlen);
        }

        if (dev_exit_flag) {
            break;
        }
    }

    return 0;
}


static void send_evt(int evt)
{
    PostThreadMessage(GetThreadId(devThreadHandle), evt, 0, 0);
}
static void CALLBACK timer_callback(HWND hwnd, UINT message, UINT_PTR iTimerID, DWORD dwTime)
{
    send_evt(WM_USER);
}


static DWORD WINAPI dev_thread(LPVOID lpParam)
{
    MSG msg;
    
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (dev_exit_flag) {
            break;
        }

        switch (msg.message) {

        case WM_USER:
        {
            timer_proc();
        }
        break;
        }


        
    }

    return 0;
}




int task_dev_start(void)
{
    dev_exit_flag = 0;

    com_init();

    timerId = SetTimer(NULL, 1, 100, timer_callback);
    devRxThreadHandle = CreateThread(NULL, 0, dev_rx_thread, NULL, 0, NULL);
    devThreadHandle = CreateThread(NULL, 0, dev_thread, NULL, 0, NULL);

    return 0;
}

int task_dev_stop(void)
{
    dev_exit_flag = 1;

    KillTimer(NULL, timerId);
    if (devRxThreadHandle) {
        WaitForSingleObject(devRxThreadHandle, INFINITE);
        CloseHandle(devRxThreadHandle);
        devRxThreadHandle = NULL;
    }

    if (devThreadHandle) {
        send_evt(WM_USER);
        WaitForSingleObject(devThreadHandle, INFINITE);
        CloseHandle(devThreadHandle);
        devThreadHandle = NULL;
    }
    

    return 0;
}

