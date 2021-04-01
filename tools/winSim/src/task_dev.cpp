#include <windows.h>
#include "dev.h"
#include "win.h"
#include "log.h"
#include "pkt.h"
#include "data.h"
#include "task.h"

#define TIMER_MS            100
#define TIMEOUT             500
#define RETRIES             (TIMEOUT/TIMER_MS)

static BYTE dev_rx_buf[1000];
static int dev_exit_flag=0;
static int paras_tx_flag=0;
static UINT_PTR timerId;
static HANDLE devRxThreadHandle, devThreadHandle;

static void com_init(void)
{
    pkt_cfg_t cfg;

    curParas = DEFAULT_PARAS;
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
static U8 com_proc(pkt_hdr_t* p, U16 len)
{
    U8 ack = 0, err;

    if (p->askAck)  ack = 1;

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
                if (p->askAck) {
                    pkt_send_ack(p->type, 0); ack = 0;
                }
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
                if (p->askAck) {
                    pkt_send_ack(p->type, 0); ack = 0;
                }
                pkt_send(TYPE_PARAS, 0, &curParas, sizeof(curParas));
                paras_tx_flag = 1;
            }
        }
        break;

        case TYPE_UPGRADE:
        {
            if (p->askAck) {
                pkt_send_ack(p->type, 0); ack = 0;
            }
            err = upgrade_proc(p->data);
        }
        break;

        default:
        {
            err = ERROR_PKT_TYPE;
        }
        break;
        }
    }

    if (ack || (err && !ack)) {
        pkt_send_ack(p->type, err);
    }

    return err;
}

static int send_stat(void)
{
    stat_t stat;
                
    stat.temp = 88.9F;
    stat.aPres = 88.9F;
    stat.dPres = 88.9F;
    stat.pump.speed = 500;
    stat.pump.current = 650.0F;
    stat.pump.temp = 44.9F;
    stat.pump.fault = 0;
    stat.pump.cmdAck = 1;
    pkt_send(TYPE_STAT, 0, &stat, sizeof(stat));
    
    return 0;
}
static void timer_proc(void)
{
    U8 i;

    if (!dev_is_opened()) {
        return;
    }

    if (!paras_tx_flag) {
        pkt_send(TYPE_PARAS, 1, &curParas, sizeof(curParas));
        paras_tx_flag = 1;
    }

    for (i = 0; i < TYPE_MAX; i++) {
        pkt_ack_check(i);
    }

    send_stat();
}



static DWORD WINAPI dev_rx_thread(LPVOID lpParam)
{
    int rlen;
    pkt_hdr_t* p = (pkt_hdr_t*)dev_rx_buf;

    while (1) {
        rlen = dev_recv(p, sizeof(dev_rx_buf));
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
    PostThreadMessage(GetThreadId(devThreadHandle), evt, NULL, NULL);
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

