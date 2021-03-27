#include <windows.h>
#include "dev.h"
#include "win.h"
#include "log.h"
#include "data.h"
#include "task.h"

static BYTE app_rx_buf[1000];
static BYTE app_tx_buf[1000];
static int app_exit_flag;
setts_t  curSetts;
sett_t   *pCurSett;

static int stat_proc(pkt_hdr_t* p)
{
    int r = 0;
    static U16 upg_pid = 0;
    stat_t* st = (stat_t*)p->data;

}
static int ack_proc(pkt_hdr_t* p)
{
    int r = 0;
    static U16 upg_pid = 0;
    stat_t* st = (stat_t*)p->data;

}
static U8 get_checksum(U8* data, U16 len)
{
    U8 sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}


static int app_data_proc(void *data, int len)
{
    U8  checksum;
    U8* buf = (U8*)data;
    pkt_hdr_t* p = (pkt_hdr_t*)data;

    if (p->magic != PKT_MAGIC) {
        log("___ pkt magic is 0x%04x, magic is wrong!\n", p->magic);
        return -1;
    }

    if (len != p->dataLen + PKT_HDR_LENGTH + 1) {
        log("___ pkt length is wrong\n");
        return -1;
    }

    checksum = get_checksum(buf, len - 1);
    if (checksum != buf[len - 1]) {
        log("___ pkt checksum is wrong!\n", checksum);
        return -1;
    }

    switch (p->type) {
        case TYPE_STAT:
        {
            log("___ TYPE_STAT\n");
            stat_t* stat = (stat_t*)p->data;
            win_stat_update(stat);
        }
        break;

        case TYPE_ACK:
        {
            log("___ TYPE_ACK\n");
        }
        break;

        case TYPE_SETT:
        {
            log("___ TYPE_SETT\n");
            memcpy(&curSetts, p->data, sizeof(curSetts));
        }
        break;

        case TYPE_UPGRADE:
        {
            log("___ TYPE_UPGRADE\n");
        }
        break;

        case TYPE_FWINFO:
        {
            log("___ TYPE_FWINFO\n");
        }
        break;

        default:
            break;
    }

    return 0;
}


static DWORD WINAPI app_rx_thread(LPVOID lpParam)
{
    int r;
    int rlen;
    
    //data_thread_run = 1;
    while (1) {
        rlen = dev_recv(app_rx_buf, sizeof(app_rx_buf));
        if (rlen > 0) {
            r = app_data_proc(app_rx_buf, rlen);
            if (app_exit_flag) {
                break;
            }
        }
    }

    return 0;
}
static DWORD WINAPI app_tx_thread(LPVOID lpParam)
{
    int r;
    int rlen;
    
    while (1) {
        if (app_exit_flag) {
            break;
        }
    }

    return 0;
}


static HANDLE appThreadHandle,appRxThreadHandle,appTxThreadHandle;
int task_app_start(void)
{
    app_exit_flag = 0;
    appRxThreadHandle = CreateThread(NULL, 0, app_rx_thread, NULL, 0, NULL);
    appTxThreadHandle = CreateThread(NULL, 0, app_tx_thread, NULL, 0, NULL);
    
    return 0;
}

int task_app_stop(void)
{
    app_exit_flag = 1;

    if (appRxThreadHandle) {
        WaitForSingleObject(appRxThreadHandle, INFINITE);
        CloseHandle(appRxThreadHandle);
        appRxThreadHandle = NULL;
    }

    if (appTxThreadHandle) {
        WaitForSingleObject(appTxThreadHandle, INFINITE);
        CloseHandle(appTxThreadHandle);
        appTxThreadHandle = NULL;
    }

    return 0;
}


