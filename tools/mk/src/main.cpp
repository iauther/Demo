
#include "myWindow.h"
#include "protocol.h"
#include "port.h"
#include "pkt.h"
#include "rbuf.h"




#define DEBUG_ON
#define CHMAX				20
#define REC_THRESHOLD_LEN   500000

#define BUF_LEN				80000
#define MY_TIMER			(WM_USER+0x01)




typedef struct {
	FILE* handle;
	int   flength;
}file_data_t;


typedef struct {
	BYTE rx[BUF_LEN];
	BYTE tx[BUF_LEN];
	
	BYTE tmp[BUF_LEN];
	BYTE pkt[BUF_LEN];

	BYTE rb[BUF_LEN];
	handle_t hrb;
}buf_data_t;

typedef struct {
	myWindow	myWin;
	U8          sysState;
	CAppModule  module;

	buf_data_t  com;
	buf_data_t  sock;
	file_data_t recFile[CHMAX];
}my_handle_t;


UINT_PTR	tmrID;
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
static void timer_func(HWND hwnd, UINT message, UINT_PTR timerID, DWORD dwTime)
{

}
static void start_timeout_timer(int ms)
{
	KillTimer(NULL, tmrID);
	tmrID = SetTimer(NULL, 1, ms, NULL);
}
static void stop_timeout_timer(void)
{
	::KillTimer(NULL, tmrID);
}


static DWORD WINAPI rxThread(LPVOID lpParam)
{
	int rlen;
	U8  checksum;
	U8* buf = myHandle.com.rx;
	pkt_hdr_t* p = (pkt_hdr_t*)buf;

	while (1) {
		rlen = port_recv(buf, BUF_LEN);
		if (rlen > 0) {

			if (p->magic != PKT_MAGIC) {
				//LOG("___ pkt magic is 0x%04x, magic is wrong!\n", p->magic);
				continue;
			}
			if (p->dataLen + PKT_HDR_LENGTH + 1 != rlen) {
				//LOG("___ pkt length is wrong\n");
				continue;
			}

			checksum = get_checksum(buf, p->dataLen + PKT_HDR_LENGTH);
			if (checksum != buf[p->dataLen + PKT_HDR_LENGTH]) {
				//LOG("___ pkt checksum is wrong!\n", checksum);
				continue;
			}

			switch (p->type) {
				case TYPE_STAT:
				{
					//stat_t* stat = (stat_t*)p->data;
				}
				break;

				default:
				break;
			}
		}
	}

	return 0;
}
static DWORD WINAPI mainThread(LPVOID lpParam)
{
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		switch (msg.message) {

			case WM_QUIT:
			break;

			case WM_PAINT:
			break;

			case WM_TIMER:
			break;
		}
	}

	return 0;
}



static void get_datatime(U8 ch, char *path, int len)
{
	SYSTEMTIME stime;
	GetLocalTime(&stime);
	snprintf(path, len, "rec_ch%02d_%04d%02d%02d_%02d%02d%02d.csv", ch, stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond);
}
void startRecFile(U8 ch)
{
	char path[100];
	file_data_t* fd = NULL;
	
	if (ch >= CHMAX) {
		print("___ ch %d is invalid, max: %d\n", ch, CHMAX);
		return;
	}

	fd = &myHandle.recFile[ch];
	if (!fd->handle) {
		get_datatime(ch, path, sizeof(path));
		fd->handle = fopen(path, "w+");
		fd->flength = 0;
	}
}
void stopRecAll(void)
{
	int i;
	file_data_t* fd = NULL;

	for (i = 0; i < CHMAX; i++) {
		fd = &myHandle.recFile[i];
		if (fd->handle) {
			fclose(fd->handle);
			fd->handle = NULL;
			fd->flength = 0;
		}
	}
}

void stopRecFile(U8 ch)
{
	file_data_t* fd = NULL;

	if (ch >= CHMAX) {
		print("___ ch %d is invalid, max: %d\n", ch, CHMAX);
		return;
	}

	if (!myHandle.recFile[ch].handle) {
		print("___ ch %d not open!\n", ch);
		return;
	}
	fd = &myHandle.recFile[ch];

	fd->handle = NULL;
	fd->flength = 0;
}
int saveFlag = 0;
void writeFile(U8 ch, void* data, int len)
{
	int i;
	U32* p32 = (U32*)data;
	file_data_t* fd = NULL;

	if (ch >= CHMAX || !data || !len) {
		print("___ invalid param: ch: %d, data: %x, len: %d\n", ch, (u32)data, len);
		return;
	}

	if (!saveFlag) {
		return;
	}

	fd = &myHandle.recFile[ch];
	startRecFile(ch);
	if (fd->handle && fd->flength < REC_THRESHOLD_LEN) {
		for (i = 0; i < len / 4; i++) {
			fprintf(myHandle.recFile[ch].handle, "0x%x, \n", p32[i]);
		}
		fd->flength += len;
	}
}




DWORD WINAPI sockRecvThread(LPVOID lpParam)
{
	SOCKET sHost = *(SOCKET*)lpParam;
	int rlen, find,wlen;
	handle_t hrb = myHandle.sock.hrb;
	BYTE* rxBuf = myHandle.sock.rx;

	rbuf_reset(hrb);
	memset(rxBuf, 0, BUF_LEN);
	while (1) {
		rlen = recv(sHost, (char*)rxBuf, BUF_LEN, 0);
		if (rlen > 0) {
			wlen = rbuf_write(hrb, rxBuf, rlen);
		}

		if (quit) {
			break;
		}
	}

	return 0;
}
DWORD WINAPI comRecvThread(LPVOID lpParam)
{
	int rlen, find, wlen;
	handle_t hrb = myHandle.com.hrb;
	BYTE* rxBuf = myHandle.com.rx;
	vector<SerialPortInfo> pList;

	pList = port_info();

	//port_open(1);
	rbuf_reset(hrb);
	memset(rxBuf, 0, BUF_LEN);
	while (1) {
		rlen = port_recv( (char*)rxBuf, BUF_LEN);
		if (rlen > 0) {
			wlen = rbuf_write(hrb, rxBuf, rlen);
		}

		if (quit) {
			break;
		}
	}

	return 0;
}




static int pkt_proc(U8 *data, int len, int buflen)
{
	int err;
	F32* p32;
	pkt_hdr_t* hdr = (pkt_hdr_t*)data;

	p32 = (F32*)hdr->data;

#ifdef DEBUG_ON
	print("hdr->magic:   0x%08x\r\n", hdr->magic);
	print("hdr->type:    0x%02x\r\n", hdr->type);
	print("hdr->flag:    %d\r\n",     hdr->flag);
	print("hdr->dataLen: %d\r\n",     hdr->dataLen);
#endif

	switch (hdr->type) {
		case TYPE_CAP:
		{
#ifdef DEBUG_ON
			ch_data_t* cd = (ch_data_t*)hdr->data;
			print("cd->ch:          %d\r\n", cd->ch);
			print("cd->data[0]: 0x%04x\r\n", cd->data[0]);
			print("cd->data[1]: 0x%04x\r\n", cd->data[1]);
			print("cd->data[2]: 0x%04x\r\n", cd->data[2]);
			print("\n");
#endif
			writeFile(cd->ch, cd->data, cd->len - sizeof(ch_data_t));
		}
		break;
	}

	return 0;
}


static DWORD WINAPI sockRecvDataProcThread(LPVOID lpParam)
{
	int i, rlen,plen;
	int err,find;
	pkt_hdr_t *hdr;
	handle_t hrb = NULL;
	BYTE* rbBuf = myHandle.sock.rb;
	BYTE* tmpBuf = myHandle.sock.tmp;
	BYTE* pktBuf = myHandle.sock.pkt;

	hrb = rbuf_init(rbBuf, BUF_LEN);
	myHandle.sock.hrb = hrb;
	while (1) {
		
		rlen = rbuf_read(hrb, tmpBuf, BUF_LEN, 0);
		if (rlen > PKT_HDR_LENGTH) {
			find = plen = 0;
			for (i = 0; i < rlen; i++) {
				hdr = (pkt_hdr_t*)(tmpBuf + i);
				if (hdr->magic == PKT_MAGIC) {
					int plen = hdr->dataLen + PKT_HDR_LENGTH;
					if (rlen-i>=plen) {
						err = pkt_check_hdr(hdr, plen, rlen-i, CHK_NONE);
						if (err) {
							print("pkt hdr error: 0x%02x\n", err);
							//return err;
							continue;
						}

						if (plen<=BUF_LEN) {
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


static DWORD WINAPI comRecvDataProcThread(LPVOID lpParam)
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
						err = pkt_check_hdr(hdr, plen, rlen - i, CHK_NONE);
						if (err) {
							print("pkt hdr error: 0x%02x\n", err);
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


//入口函数
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    myHandle.module.Init(NULL, hInstance);  //初始化模型

    CRect rc = CRect(POINT{ 400, 100 }, POINT{ 1200, 700 });
	myHandle.myWin.Create(NULL, rc, _T("miniTest"), WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX);

	CreateThread(NULL, NULL, sockRecvDataProcThread, NULL, 0, NULL);
	CreateThread(NULL, NULL, comRecvDataProcThread, NULL, 0, NULL);

	CreateThread(NULL, 0, comRecvThread, NULL, 0, NULL);

    while (GetMessage(&msg, NULL, 0,0)>0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

	myHandle.module.Term();

    return 0;
}



