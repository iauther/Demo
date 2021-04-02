#include "task.h"


#define TIMER_MS            100
#define TIMEOUT             500
#define RETRIES             (TIMEOUT/TIMER_MS)


static BYTE app_rx_buf[1000];
static int app_exit_flag;
static int paras_rx_flag = 0;
static UINT_PTR timerId;
static HANDLE appThreadHandle, appRxThreadHandle;
extern paras_t DEFAULT_PARAS;

static void com_init(void)
{
    pkt_cfg_t cfg;

    curParas = DEFAULT_PARAS;

    cfg.retries = RETRIES;
    pkt_init(&cfg);
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

        case TYPE_STAT:
        {
            stat_t* stat = (stat_t*)p->data;

            LOG("_____ stat temp: %d, aPres: %dkpa, dPres: %dkpa\n", stat->temp, stat->aPres, stat->dPres);
            win_stat_update(stat);
        }
        break;

        case TYPE_ACK:
        {
            ack_t* ack = (ack_t*)p->data;

            LOG("___ ack recived, type: %d\n", ack->type);
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
                    memcpy(&curParas.setts.sett, p->data, sizeof(curParas.setts.sett));
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
            LOG("___ paras recived\n");
            if (p->dataLen > 0) {
                if (p->dataLen == sizeof(paras_t)) {
                    paras_t* paras = (paras_t*)p->data;
                    curParas = *paras;
                }
                else {
                    err = ERROR_PKT_LENGTH;
                    LOG("___ paras length is wrong\n");
                }
            }
            else {
                if (p->askAck) {
                    pkt_send_ack(p->type, 0); ack = 0;
                }
                pkt_send(TYPE_PARAS, 0, &curParas, sizeof(curParas));
                paras_rx_flag = 1;
            }
        }
        break;

        case TYPE_UPGRADE:
        {
            if (p->askAck) {
                pkt_send_ack(p->type, 0); ack = 0;
            }
            //err = upgrade_proc(p->data);
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
static void timer_proc(void)
{
    U8 i;

    if (!port_is_opened()) {
        return;
    }

    if (!paras_rx_flag) {
        pkt_send(TYPE_PARAS, 0, NULL, 0);
        paras_rx_flag = 1;
    }

    for (i = 0; i < TYPE_MAX; i++) {
        pkt_ack_timeout_check(i);
    }
}

static DWORD WINAPI app_rx_thread(LPVOID lpParam)
{
    int rlen;
    pkt_hdr_t* p = (pkt_hdr_t*)app_rx_buf;
    
    while (1) {
        rlen = port_recv(p, sizeof(app_rx_buf));
        if (rlen > 0) {
            com_proc(p, rlen); 
        }

        if (app_exit_flag) {
            break;
        }
    }

    return 0;
}

static void send_evt(int evt)
{
    PostThreadMessage(GetThreadId(appThreadHandle), evt, 0, 0);
}
static void CALLBACK timer_callback(HWND hwnd, UINT message, UINT_PTR iTimerID, DWORD dwTime)
{
    send_evt(WM_USER);
}
static DWORD WINAPI app_thread(LPVOID lpParam)
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (app_exit_flag) {
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



int task_app_start(void)
{
    app_exit_flag = 0;

    timerId = SetTimer(NULL, 1, 100, timer_callback);
    appRxThreadHandle = CreateThread(NULL, 0, app_rx_thread, NULL, 0, NULL);
    appThreadHandle = CreateThread(NULL, 0, app_thread, NULL, 0, NULL);
    
    return 0;
}

int task_app_stop(void)
{
    app_exit_flag = 1;

    KillTimer(NULL, timerId);
    if (appRxThreadHandle) {
        WaitForSingleObject(appRxThreadHandle, INFINITE);
        CloseHandle(appRxThreadHandle);
        appRxThreadHandle = NULL;
    }

    if (appThreadHandle) {
        send_evt(WM_USER);
        WaitForSingleObject(appThreadHandle, INFINITE);
        CloseHandle(appThreadHandle);
        appThreadHandle = NULL;
    }

    return 0;
}


