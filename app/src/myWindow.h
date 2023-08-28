
#pragma once

#include "wtl.h"
#include "log.h"


enum {
	ID_PORT =0xff00,

	ID_OPEN,
    ID_CALI,
	ID_START,

	ID_LOG_EN,
	ID_LOG_CLR,
	ID_LOG_SAV,
};


//#define USE_MQTT


int quit = 0;
static HANDLE ackEvent;

#ifdef USE_MQTT
#include "myMqtt.h"
#include "mqtt_sign.h"
static mqtt_sign_t sign;
static myMqtt mMqtt;

char* mqttTopic = (char*)"/h471OT98L3W/gasServer/user/XXX";
static dev_meta_t devMeta = {
	"h471OT98L3W",
	"6WKDXY0lXkIaa6Cv",
	"gasServer",
	"35aee223ab3482f2e24a53435fe60ec7",

	"iot-06z00ers5w6yi1p.mqtt.iothub.aliyuncs.com",
	1883,
};
#endif

class myPage : public CFileDialogImpl<myPage>,
	           public CDialogImpl<myPage>
{
public:
	CTabCtrl mTabs;
};


class myWindow : public CWindowImpl<myWindow, CWindow>
{

public:
	myWindow();
	~myWindow();

	CFont   gFont;
	CPagerCtrl page;
	CComboBox  port;
	//CComboBoxEx  port;
	CSplitterWindow    vSplit;		//主窗口，垂直分割
	CHorSplitterWindow hSplit1;		//右窗口，水平分割
	CHorSplitterWindow hSplit2;
	CPaneContainer     wavPane, infPane, ctlPane, dbgPane;
	CButton btOpen, btCali, btStart;
	CButton btEna, btSav, btClr;

	int dev_opened=0;
	int dev_started=0;

	int log_started=0;
	int log_enabled=0;



	int proto = 1;		//0: tcp   1: mqtt

	BEGIN_MSG_MAP(myWindow)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)

		//MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_SIZING, OnSize)

		//MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
		//MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)

		COMMAND_HANDLER(ID_OPEN, BN_CLICKED, OnBtnClicked)
		COMMAND_HANDLER(ID_CALI, BN_CLICKED, OnBtnClicked)
		COMMAND_HANDLER(ID_START, BN_CLICKED, OnBtnClicked)
		COMMAND_HANDLER(ID_LOG_EN, BN_CLICKED, OnBtnClicked)
		COMMAND_HANDLER(ID_LOG_CLR, BN_CLICKED, OnBtnClicked)
		COMMAND_HANDLER(ID_LOG_SAV, BN_CLICKED, OnBtnClicked)

		//NOTIFY_HANDLER(IDC_TABCTRLDOWN, TCN_SELCHANGE, OnSelChangeTab)

		//MESSAGE_HANDLER(WM_PAINT, OnPaint)
		//MSG_WM_CLOSE(OnClose)
	END_MSG_MAP()


	void gblInit(void)
	{
		gFont.CreateFont(16, 8, 0, 0, FW_THIN, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT("Arial"));

		//gFont.CreatePointFont(220, TEXT("微软雅黑"));

		//CClientDC dc(m_hWnd);
		//dc.SetTextColor(RGB(0, 0, 0));
	}



	/*
		https://www.orcode.com/article/WTL_20113298.html
	*/
	void splitInit(void)
	{
		CRect rc;
		GetClientRect(&rc);

		//创建垂直分割窗口
		vSplit.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE);
		vSplit.m_bFullDrag = false;
		//vSplit.m_cxySplitBar = 4;
		vSplit.SetSplitterPos(rc.right * 2 / 3);		//wav占水平窗口2/3

		wavPane.Create(vSplit.m_hWnd);
		wavPane.SetTitle(_T("wave"));
		wavPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

		vSplit.GetClientRect(&rc);
		hSplit1.Create(vSplit.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		hSplit1.SetSplitterPos(rc.bottom / 3);			//info占垂直窗口1/3

		vSplit.SetSplitterPanes(wavPane, hSplit1);
		//////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////////////////////////////////
		hSplit1.GetClientRect(&rc);
		hSplit2.Create(hSplit1.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		hSplit2.SetSplitterPos(rc.bottom / 2);			//ctl和dbg各占剩下垂直窗口1/2

		infPane.Create(hSplit1.m_hWnd);
		infPane.SetTitle(_T("inf"));
		infPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

		ctlPane.Create(hSplit2.m_hWnd);
		ctlPane.SetTitle(_T("ctl"));
		ctlPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

		dbgPane.Create(hSplit2.m_hWnd);
		dbgPane.SetTitle(_T("dbg"));
		dbgPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

		hSplit1.SetSplitterPanes(infPane, hSplit2);
		hSplit2.SetSplitterPanes(ctlPane, dbgPane);

#ifdef USE_MQTT
		mMqtt.set_print(NULL);
#endif
	}

	void port_update(void)
	{
		port.ShowDropDown(FALSE);
		port.AddString("123");
		port.AddString("456");
		port.AddString("444");
		//port.SetDroppedWidth(80);
		port.SetCurSel(2);

	}


	void ctlPaneInit(void)
	{
		CRect rc;

#define BTN_WIDTH   80
#define BTN_HEIGHT  30

		ctlPane.GetClientRect(&rc);

		rc.left = 20;
		rc.right = rc.left + BTN_WIDTH;
		rc.top = 40;
		rc.bottom = rc.top + BTN_HEIGHT;
		//port.Create(ctlPane.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS, NULL, ID_PORT, NULL);
		//port_update();

		//device control buttons
		rc.left = 20;
		rc.right = rc.left + BTN_WIDTH;
		rc.top = 80;
		rc.bottom = rc.top + BTN_HEIGHT;
		btOpen.Create(ctlPane.m_hWnd, rc, "dev open", WS_CHILD | WS_VISIBLE, NULL, ID_OPEN, NULL);
		btOpen.SetFont(gFont);

		rc.left = 120;
		rc.right = rc.left + BTN_WIDTH;
		btCali.Create(ctlPane.m_hWnd, rc, "dev cali", WS_CHILD | WS_VISIBLE, NULL, ID_CALI, NULL);
		btCali.SetFont(gFont);

		rc.left = 220;
		rc.right = rc.left + BTN_WIDTH;
		btStart.Create(ctlPane.m_hWnd, rc, "dev start", WS_CHILD | WS_VISIBLE, NULL, ID_START, NULL);
		btStart.SetFont(gFont);

		//log control button
		rc.left = 20;
		rc.top = 120;
		rc.right = rc.left + BTN_WIDTH;
		rc.bottom = rc.top + BTN_HEIGHT;
		btEna.Create(ctlPane.m_hWnd, rc, "log off", WS_CHILD | WS_VISIBLE, NULL, ID_LOG_EN, NULL);
		btEna.SetFont(gFont);

		rc.left = 120;
		rc.right = rc.left + BTN_WIDTH;
		btClr.Create(ctlPane.m_hWnd, rc, "log clr", WS_CHILD | WS_VISIBLE, NULL, ID_LOG_CLR, NULL);
		btClr.SetFont(gFont);

		rc.left = 220;
		rc.right = rc.left + BTN_WIDTH;
		btSav.Create(ctlPane.m_hWnd, rc, "log save", WS_CHILD | WS_VISIBLE, NULL, ID_LOG_SAV, NULL);
		btSav.SetFont(gFont);

	}

	void dbgPaneInit(void)
	{
		CRect rc;

		dbgPane.GetClientRect(&rc);

		log_init(dbgPane.m_hWnd, &rc);

		dbgPane.SetClient(log_get_hwnd());
	}



	void paneInit(void)
	{
		ctlPaneInit();
		dbgPaneInit();
	}

#ifdef USE_MQTT
	static void mqtt_msg_published(void* context, int dt, int packet_type, MQTTProperties* properties, enum MQTTReasonCodes reasonCode)
	{

	}
	static int mqtt_msg_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* m)
	{
		int i;
		//vector<char>mqttBuffer;

		LOGD("recv: %s\n", (char*)m->payload);

		MQTTClient_freeMessage(&m);
		MQTTClient_free(topicName);

		return 1;
	}


	static void mqtt_disconn(void* context, MQTTProperties* properties, enum MQTTReasonCodes reasonCode)
	{
		mMqtt.conn(devMeta.hostName, devMeta.port, sign.clientId, sign.username, sign.password);
	}
#endif

	static void data_recv_fn(void* buf, int len)
	{

	}



	int dev_open(void)
	{
		if (proto == 0) {

		}
		else {
#ifdef USE_MQTT
			mqtt_sign(&devMeta, &sign, SIGN_HMAC_SHA1);
			mMqtt.conn(devMeta.hostName, devMeta.port, sign.clientId, sign.username, sign.password);

			mqtt_callback_t cb;

			memset(&cb, 0, sizeof(cb));
			cb.arrived = mqtt_msg_arrived;
			cb.published = mqtt_msg_published;
			cb.disconn = mqtt_disconn;
			mMqtt.set_cb(&cb);
			mMqtt.subscribe(mqttTopic);
#endif
		}

		//SetTimer(88, 200, NULL);

		return 0;
	}

	int dev_close(void)
	{
		if (proto == 0) {
			//mSock.disconn();
		}
		else {
#ifdef USE_MQTT
			mMqtt.disconn();
#endif
		}

		//KillTimer(88);

		return 0;
	}


	void mqtt_write()
	{
#ifdef USE_MQTT
		if (mMqtt.is_connected()) {
			memset(tmp, 0, sizeof(tmp));
			sprintf(tmp, "XXXX %d", pub_cnt++);

			LOGD("pub: %s\n", tmp);
			r = mMqtt.publish(mqttTopic, tmp, 1);

		}
		else {
			LOGD("___mqtt not connected!\n");
		}
#endif
	}

		
		LRESULT OnBtnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
		{
			int r;
			extern int io_write(U8 type, U8 nAck, void* data, int len);

			CButton btn = hWndCtl;

			switch (wID) {
			case ID_OPEN:
			{
				if (dev_opened == 0) {
					dev_open();
				}
				else {
					dev_close();
				}
				dev_opened = dev_opened?0:1;
				btOpen.SetWindowText(dev_opened?"close":"open");
			}
			break;

			case ID_CALI:
			{
				cali_t ca;

				ca.ch = 0;
				ca.rms = 40.0f;
				ca.bias = 0.0f;
				r = io_write(TYPE_CALI, 0, &ca, sizeof(ca));
			}
			break;

			case ID_START:
			{
				capture_t cap = {0, !dev_started};

				r = io_write(TYPE_CAP, 0, &cap, sizeof(cap));
				if (r==0) {
					dev_started = dev_started?0:1;

					btStart.SetWindowText(dev_started?"stop":"start");
				}
			}
			break;

			case ID_LOG_EN:
			{
				if (log_is_enable()) {
					log_enable(0);
				}
				else {
					log_enable(1);
				}
			}
			break;

			case ID_LOG_CLR:
			{
				log_clear();
			}
			break;

			case ID_LOG_SAV:
			{
				//extern void writeFile(U8 ch, void* data, int len);
			}
			break;
			}

			return 0;
		}


		LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
		{
			gblInit();
			splitInit();
			paneInit();

			return 0;
		}

		int t_cnt = 0;
		LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
		{


			LOGD("____cnt: %d\n", t_cnt++);

			return 0;
		}

		LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
		{

			DestroyWindow();
			PostQuitMessage(0);

			return 0;
		}

		LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			//CPaintDC dc(m_hWnd);
			//CMemoryDC mdc(dc,dc.m_ps.rcPaint);
			
			

			return 0;
		}

		LRESULT DoPaint(CDCHandle dc)
		{
			CRect rc;
			Gdiplus::Point pt1,pt2;
			Gdiplus::Graphics graphics(dc);
			Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255), 1);

			GetClientRect(&rc);

			graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
			graphics.DrawLine(&pen, pt1, pt2);

			return 0;
		}

		LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{return 0;}
		LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)	{return 0;}

		
		void UpdateLayout(BOOL bResizeBars = TRUE)
		{

		}

		LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			//wavPane.OnSize(uMsg, wParam, lParam, bHandled);
			//infPane.OnSize(uMsg, wParam, lParam, bHandled);
			//ctlPane.OnSize(uMsg, wParam, lParam, bHandled);
			//dbgPane.OnSize(uMsg, wParam, lParam, bHandled);

			//infPane.OnPaint(uMsg, wParam, lParam, bHandled);
			//ctlPane.OnPaint(uMsg, wParam, lParam, bHandled);
			//dbgPane.OnPaint(uMsg, wParam, lParam, bHandled);

			//RedrawWindow();
			//vSplit.OnPaint(uMsg, wParam, lParam, bHandled);

			//
			
			return 0;
		}


	private:
		CButton		cmd[40];
		CComboBox	aesKeyLen;
		CComboBox	aesMode;
		CStatic     label;
		CString     str;
		CListBox    lbox;
		CScrollBar  sbar;

};



myWindow::myWindow()
{
	ackEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
}


myWindow::~myWindow()
{
	dev_close();
	CloseHandle(ackEvent);
}































