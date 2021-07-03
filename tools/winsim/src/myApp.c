#include "types.h"
#include "task.h"
#include "paras.h"
#include "com.h"
#include "log.h"

#define LEAP_INV_MS         5000
#define LEAP_CNT            (LEAP_INV_MS/ACK_POLL_MS)


#define WM_DIY              (WM_USER+100)


static int app_exit_flag;
static int app_thread_running = 0;
static UINT_PTR timerId;
static HANDLE appThreadHandle, appRxThreadHandle;
extern paras_t DEFAULT_PARAS;


const char* typeString2[TYPE_MAX] = {
    "TYPE_CMD",
    "TYPE_STAT",
    "TYPE_ACK",
    "TYPE_SETT",
    "TYPE_PARAS",
    "TYPE_ERROR",
    "TYPE_UPGRADE",
    "TYPE_LEAP",
};
void err_print(U8 type, U8 err)
{
    switch (err) {
    case ERROR_NONE:
        //LOG("_____ pkt is ok\n");
        break;
    case ERROR_DEV_PUMP_RW:
        LOG("_____ pump read/write error\n");
        break;
    case ERROR_DEV_MS4525_RW:
        LOG("_____ ms4525 read/write error\n");
        break;
    case ERROR_DEV_E2PROM_RW:
        LOG("_____ eeprom read/write error\n");
        break;
    case ERROR_PKT_TYPE:
        LOG("_____ pkt type is wrong\n");
        break;
    case ERROR_PKT_MAGIC:
        LOG("_____ pkt magic is wrong\n");
        break;
    case ERROR_PKT_LENGTH:
        LOG("_____ pkt length is wrong\n");
        break;
    case ERROR_PKT_CHECKSUM:
        LOG("_____ pkt checksum is wrong\n");
        break;
    case ERROR_PKT_TYPE_UNSUPPORT:
        LOG("_____ pkt %s unsupport\n", typeString2[type]);
        break;
    case ERROR_DAT_TIMESET:
        LOG("_____ para timeset is wrong\n");
        break;
    case ERROR_DAT_VACUUM:
        LOG("_____ para vacuum is wrong\n");
        break;
    case ERROR_DAT_VOLUME:
        LOG("_____ para volume is wrong\n");
        break;
    case ERROR_FW_INFO_VERSION:
        LOG("_____ fw version is wrong\n");
        break;
    case ERROR_FW_INFO_BLDTIME:
        LOG("_____ fw buildtime is wrong\n");
        break;
    case ERROR_FW_PKT_ID:
        LOG("_____ fw pkt id is wrong\n");
        break;
    case ERROR_FW_PKT_LENGTH:
        LOG("_____ fw pkt length is wrong\n");
        break;
    case ERROR_ACK_TIMEOUT:
        LOG("_____ %s, ack is timeout\n", typeString2[type]);
        break;
    default:
        LOG("_____ unknow error: %d\n", err);
        break;
    }
}
void pkt_print(pkt_hdr_t* p, U16 len)
{
    LOG("\n_____ pkt length:   %d\n", len);

    LOG("_____ pkt.magic: 0x%08x\n", p->magic);
    LOG("_____ pkt.type:  0x%02x\n", p->type);
    LOG("_____ pkt.askAck: %d\n", p->askAck);
    LOG("_____ pkt.flag:  0x%02x\n", p->flag);
    LOG("_____ pkt.dataLen: %d\n", p->dataLen);

    switch (p->type) {
    case TYPE_CMD:
    {
        LOG("_____ type_cmd\n");
        if (p->dataLen == sizeof(cmd_t)) {
            cmd_t* cmd = (cmd_t*)p->data;
            LOG("_____ cmd.cmd: 0x%08x, cmd.para: %d\n", cmd->cmd, cmd->para);
        }
    }
    break;

    case TYPE_STAT:
    {
        LOG("_____ type_stat\n");
        if (p->dataLen == sizeof(stat_t)) {
            stat_t* cmd = (cmd_t*)p->data;

        }
    }
    break;

    case TYPE_ACK:
    {
        LOG("_____ type_ack\n");
        if (p->dataLen == sizeof(ack_t)) {
            ack_t* ack = (ack_t*)p->data;
            LOG("_____ ack.type: %s\n", typeString2[ack->type]);
        }
    }
    break;

    case TYPE_SETT:
    {
        LOG("_____ type_sett\n");
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
            LOG("_____ type_paras\n");
            LOG("_____ version: %s\n", paras->fwInfo.version);
            LOG("_____ build time: %s\n", paras->fwInfo.bldtime);
        }
    }
    break;

    case TYPE_ERROR:
    {
        if (p->dataLen == sizeof(error_t)) {
            error_t* err = (error_t*)p->data;
            LOG("_____ type_error\n");
        }
    }
    break;

    case TYPE_UPGRADE:
    {
        if (p->dataLen == sizeof(upgrade_pkt_t)) {
            upgrade_pkt_t* up = (upgrade_pkt_t*)p->data;
            LOG("_____ type_upgrade\n");
            LOG("_____ pkt.pkts: %d\n", up->pkts);
            LOG("_____ pkt.pid:  %d\n", up->pid);
            LOG("_____ pkt.dataLen: %d\n", up->dataLen);
        }
    }
    break;

    case TYPE_LEAP:
    {
        if (p->dataLen == 0) {
            LOG("_____ type_leap\n");
        }
    }
    break;

    default:
    {
        LOG("_____ invalid type, %d\n", p->type);
    }
    break;

    }

}
//////////////////////////////////////////////////


extern int port_opened;
extern int upgrade_start;
static U8 timer_proc(void)
{
    int r;
    U8 i,err=0;
    static U32 cnt = 1;

    if (!port_opened) {
        return 0;
    }

    //if (curStat.sysState != STAT_UPGRADE) {
    if(!upgrade_start) {
        //err = com_send_data(TYPE_STAT, 0, NULL, 0);
        if (cnt % LEAP_CNT == 0) {
            //err = com_send_data(TYPE_LEAP, 1, NULL, 0);
            //LOG("____ send a leap\n");
        }

        //err = com_ack_poll();
    }

    cnt++;

    return err;
}

static DWORD WINAPI app_rx_thread(LPVOID lpParam)
{
    U8  err;
    int rlen;
    U16 rxbuflen;
    U8* rxbuf=NULL;
    pkt_hdr_t* p;

    com_get_buf(&rxbuf, &rxbuflen);
    p = rxbuf;
    while (1) {
        rlen = port_recv(rxbuf, rxbuflen);
        if (rlen > 0) {
           //pkt_print(p, rlen);
           err = com_data_proc(rxbuf, rlen);
           err_print(p, err);
        }

        if (app_exit_flag) {
            break;
        }
    }

    return 0;
}

static void app_post_evt(int evt, void *ptr)
{
    if (app_thread_running) {
        PostThreadMessage(GetThreadId(appThreadHandle), evt, ptr, 0);
    }
}
static void CALLBACK timer_callback(HWND hwnd, UINT message, UINT_PTR iTimerID, DWORD dwTime)
{
    app_post_evt(WM_USER, NULL);
}


static void dummy_fun(U8* data, U16 len)
{

}
static DWORD WINAPI app_thread(LPVOID lpParam)
{
    U8  err;
    MSG msg;

    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
    app_thread_running = 1;

    com_init(dummy_fun, ACK_POLL_MS);

    while (GetMessage(&msg, NULL, 0, 0)) {
        //TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (app_exit_flag) {
            break;
        }

        switch (msg.message) {

            case WM_USER:
            {
                err = timer_proc();
            }
            break;

            case WM_DIY:
            {
                /*
                msg_data_t* ptr = (msg_data_t*)msg.wParam;
                if (ptr) {
                    pkt_send(ptr->type, ptr->nAck, ptr->data, ptr->dlen);
                    free(ptr);
                }
                */
            }
            break;
        } 
    }

    return 0;
}



int task_app_start(void)
{
    app_exit_flag = 0;
    app_thread_running = 0;

    timerId = SetTimer(NULL, 1, ACK_POLL_MS, timer_callback);
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
        app_post_evt(WM_USER, NULL);
        WaitForSingleObject(appThreadHandle, INFINITE);
        CloseHandle(appThreadHandle);
        appThreadHandle = NULL;
    }

    return 0;
}



