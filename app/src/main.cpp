
#include "protocol.h"
#include "myLog.h"
#include "myFile.h"
#include "myWindow.h"
#include "pkt.h"
#include "rbuf.h"
#include "comm.h"
#include "log.h"

#define DEBUG_ON
#define USE_SERIAL



#define BUF_LEN				80000
#define MY_TIMER			(WM_USER+0x01)


#ifdef USE_SERIAL
U8 chkID = CHK_SUM;
#else
U8 chkID = CHK_NONE
#endif

typedef struct {
	BYTE rx[BUF_LEN];
	BYTE tx[BUF_LEN];
	
	BYTE tmp[BUF_LEN];
	BYTE pkt[BUF_LEN];

	BYTE rb[BUF_LEN];
	handle_t hrb;
}buf_data_t;

typedef struct {
	myWindow	 myWin;

	U8          sysState;
	CAppModule  module;

	buf_data_t  com;
	buf_data_t  sock;

	myFile      mFile;
	
	handle_t    hcomm;
}my_handle_t;

static UINT_PTR	tmrID;
static my_handle_t myHandle;


static U8 get_checksum(U8* data, U16 len)
{
	U8 sum = 0;
	for (int i = 0; i < len; i++) {
		sum += data[i];
	}
	return sum;
}
static void post_event(DWORD thdID, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	PostThreadMessage(thdID, Msg, wParam, lParam);
}



static int pkt_proc(U8 *data, int len, int buflen)
{
	int err;
	F32* p32;
	pkt_hdr_t* hdr = (pkt_hdr_t*)data;

	p32 = (F32*)hdr->data;

#ifdef DEBUG_ON
	LOGD("hdr->magic:   0x%08x\n", hdr->magic);
	LOGD("hdr->type:    0x%02x\n", hdr->type);
	LOGD("hdr->flag:    %d\n",     hdr->flag);
	LOGD("hdr->dataLen: %d\n",     hdr->dataLen);
#endif

	switch (hdr->type) {
		case TYPE_CAP:
		{
#ifdef DEBUG_ON
			ch_data_t* cd = (ch_data_t*)hdr->data;
			LOGD("cd->ch:          %d\n", cd->ch);
			LOGD("cd->data[0]: 0x%04x\n", cd->data[0]);
			LOGD("cd->data[1]: 0x%04x\n", cd->data[1]);
			LOGD("cd->data[2]: 0x%04x\n", cd->data[2]);
			LOGD("\n");
#endif
			//myHandle.mFile.rec_file(cd->ch, cd->data, cd->dlen - sizeof(ch_data_t));
		}
		break;

		case TYPE_CALI:
		{
			cali_t *ca= (cali_t*)hdr->data;
			
			LOGD("ch: %d\n", ca->ch);
			LOGD("ch: %f\n", ca->rms);
			LOGD("bias: %f\n", ca->bias);
			LOGD("coef.a: %f\n", ca->coef.a);
			LOGD("coef.b: %f\n", ca->coef.b);
		}
		break;
	}

	return 0;
}


int io_init(void)
{
#ifdef USE_SERIAL
	myHandle.hcomm = comm_init(PORT_UART, NULL);
#else
	myHandle.hcomm = comm_init(PORT_NET, NULL);
#endif

	return myHandle.hcomm ? 0 : -1;
}
int io_deinit(void)
{
	comm_deinit(myHandle.hcomm);

	return 0;
}

int io_read(void* buf, int len)
{
	return comm_recv_data(myHandle.hcomm, buf, len);
}

int io_write(U8 type, U8 nAck, void* data, int len)
{
	return comm_send_data(myHandle.hcomm, NULL, type, nAck, data, len);
}



static DWORD dataRecvThread(LPVOID lpParam)
{
	int rlen, find, wlen;
	handle_t hrb = myHandle.com.hrb;
	BYTE* rxBuf = myHandle.com.rx;

	io_init();

	rbuf_reset(hrb);
	memset(rxBuf, 0, BUF_LEN);
	while (1) {
		rlen = io_read((char*)rxBuf, BUF_LEN);
		if (rlen > 0) {
			wlen = rbuf_write(hrb, rxBuf, rlen);
		}

		if (quit) {
			break;
		}
	}
	io_deinit();

	return 0;
}

static DWORD dataProcThread(LPVOID lpParam)
{
	int i, rlen, plen;
	int err, find;
	pkt_hdr_t* hdr;
	handle_t hrb = NULL;
	BYTE* rbBuf = myHandle.com.rb;
	BYTE* tmpBuf = myHandle.com.tmp;
	BYTE* pktBuf = myHandle.com.pkt;

	hrb = rbuf_init(rbBuf, BUF_LEN);
	myHandle.com.hrb = hrb;
	while (1) {
		rlen = rbuf_read(hrb, tmpBuf, BUF_LEN, 0);
		if (rlen > PKT_HDR_LENGTH) {
			find = plen = 0;
			for (i = 0; i < rlen; i++) {
				hdr = (pkt_hdr_t*)(tmpBuf + i);
				if (hdr->magic == PKT_MAGIC) {
					int plen = hdr->dataLen + PKT_HDR_LENGTH;
					if (rlen - i >= plen) {
						err = pkt_check_hdr(hdr, plen, rlen - i, chkID);
						if (err) {
							LOGE("pkt hdr error: 0x%02x\n", err);
							//return err;
							continue;
						}

						if (plen <= BUF_LEN) {
							memcpy(pktBuf, hdr, (plen));
							rbuf_read_shift(hrb, plen);
							find = 1;
							break;
						}
					}
				}
			}

			if (find) {
				pkt_proc(pktBuf, plen, plen);
			}
			//print(_T("___\n"));
		}

	}

	return 0;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    myHandle.module.Init(NULL, hInstance);  //初始化模型
	//AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_DATE_CLASSES);

    CRect rc = CRect(POINT{ 400, 100 }, POINT{ 1400, 700 });
	myHandle.myWin.Create(NULL, rc, _T("test"), WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);

	CreateThread(NULL, NULL, dataRecvThread, NULL, 0, NULL);
	//CreateThread(NULL, NULL, dataProcThread, NULL, 0, NULL);

    while (GetMessage(&msg, NULL, 0,0)>0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

	myHandle.module.Term();

    return 0;
}



