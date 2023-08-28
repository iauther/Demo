
#pragma once

#include "wtl.h"
#include "myTcp.h"
#include "myMqtt.h"
#include "mqtt_sign.h"

enum {
	ID_CONN =0xff00,
    ID_CALI,
	ID_LOG_CLR,
	ID_LOG_STOP,
	ID_LOG_SAVE,
	ID_DAT_SAVE,
	ID_TRIGGER,
};


#define DBG_BUF_LEN   1000
static TCHAR dbgBuf[DBG_BUF_LEN];

int quit = 0;
int printEnableFlag = 1;
static HANDLE ackEvent;
static CEdit debug;
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

static int char2wchar(wchar_t* wchar, int wlen, char* cchar)
{
	int xlen = MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), NULL, 0);
	if (xlen >= wlen - 1) {
		xlen = wlen - 1;
	}
	MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), wchar, xlen);
	wchar[xlen] = '\0';

	return 0;
}

static int is_tailof(TCHAR *src, const TCHAR *str)
{
	int len1 = _tcslen(src);
	int len2 = _tcslen(str);
	int cmp = _tcscmp(src+len1-len2, str);

	return (cmp == 0) ? 1 : 0;

}
void print(const char* format, ...)
{
	int n = 0;
	va_list list;
	TCHAR fmt[200];
	SCROLLINFO ScroInfo;

	if (!printEnableFlag) {
		return;
	}

	//char2wchar(fmt, sizeof(fmt), (char*)format);

	va_start(list, format);
	_vstprintf_s(dbgBuf, format, list);
	va_end(list);

	//支持\r\n和\n换行
	if (is_tailof(dbgBuf, "\n")) {
		TCHAR* x = _tcsrchr(dbgBuf, '\n');
		if (x) _tcscpy(x, "\r\n");
	}

	//char2wchar(tmpBuf, sizeof(tmpBuf), tmp);
	//debug.AppendText(dbgBuf, 1, 0);
	//debug.SetText(dbgBuf);

	int i = debug.GetWindowTextLength();


#if 1
	int iTextLen = debug.GetWindowTextLength();
	debug.SetSel(iTextLen, iTextLen, FALSE);//移动光标到最后
	debug.SetFocus(); //移动光标到最后
	debug.ReplaceSel(dbgBuf);  //在光标的位置加入最新的输出日志行
	debug.LineScroll(debug.GetLineCount());
#endif
	//debug.GetScrollInfo(SB_VERT, &ScroInfo);
	//debug.SetScrollPos(SB_VERT, ScroInfo.nMax);
	//debug.LineScroll(ScroInfo.nMax);
}


class myPage : public CFileDialogImpl<myPage>,
	           public CDialogImpl<myPage>
{
public:
	CTabCtrl mTabs;
};


class myFrame : public CFrameWindowImpl<myFrame>,
                public CUpdateUI<myFrame>,
		        public CMessageFilter, 
		        public CIdleHandler
{
    //DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)
    
	public:
		myFrame();
		~myFrame();

		CPagerCtrl page;
		//CWtlsnapsplitterView m_view;
		CSplitterWindow    vSplit;		//主窗口，垂直分割
		CHorSplitterWindow hSplit1;		//右窗口，水平分割
		CHorSplitterWindow hSplit2;
		CPaneContainer     wavPane, infPane, ctlPane, dbgPane;
		CButton btConn, btCali, btSav, logClr;

		int proto = 1;		//0: tcp   1: mqtt
		myTcp mTcp;

        virtual BOOL PreTranslateMessage(MSG* pMsg)
    	{
    		if(CFrameWindowImpl<myFrame>::PreTranslateMessage(pMsg))
    			return TRUE;

    		//return m_view.PreTranslateMessage(pMsg);
    	}

    	virtual BOOL OnIdle()
    	{
    		UIUpdateToolBar();
    		return FALSE;
    	}

        BEGIN_UPDATE_UI_MAP(myFrame)
    		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
    		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
    	END_UPDATE_UI_MAP()

		BEGIN_MSG_MAP(myFrame)
			MESSAGE_HANDLER(WM_CREATE, OnCreate)
			MESSAGE_HANDLER(WM_CLOSE, OnClose)
			MESSAGE_HANDLER(WM_TIMER, OnTimer)
			//MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
			//MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)

			COMMAND_HANDLER(ID_CONN,     BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_CALI,     BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_DAT_SAVE, BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_LOG_CLR,  BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_LOG_STOP, BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_LOG_SAVE, BN_CLICKED,  OnBtnClicked)
			COMMAND_HANDLER(ID_TRIGGER,  BN_CLICKED,  OnBtnClicked)

			//NOTIFY_HANDLER(IDC_TABCTRLDOWN, TCN_SELCHANGE, OnSelChangeTab)

			//MESSAGE_HANDLER(WM_PAINT, OnPaint)
			//MSG_WM_CLOSE(OnClose)
			
			CHAIN_MSG_MAP(CUpdateUI<myFrame>)
		    CHAIN_MSG_MAP(CFrameWindowImpl<myFrame>)
			
			
		END_MSG_MAP()

		/*
			https://www.orcode.com/article/WTL_20113298.html
		*/
		void splitInit(void)
		{
			CRect rc;
			GetClientRect(&rc);

			//create root splitter window
			vSplit.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE);
			vSplit.m_bFullDrag = false;
			//vSplit.m_cxySplitBar = 4;
			vSplit.SetSplitterPos(rc.right * 2 / 3);

			wavPane.Create(vSplit.m_hWnd);
			wavPane.SetTitle(_T("wave"));
			wavPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

			vSplit.GetClientRect(&rc);
			hSplit1.Create(vSplit.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
			hSplit1.GetClientRect(&rc);
			hSplit1.SetSplitterPos(rc.bottom / 3);

			vSplit.SetSplitterPanes(wavPane, hSplit1);


			hSplit1.GetClientRect(&rc);
			hSplit2.Create(hSplit1.m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
			hSplit2.GetClientRect(&rc);
			hSplit2.SetSplitterPos(rc.bottom / 2);

			infPane.Create(hSplit1.m_hWnd);
			infPane.SetTitle(_T("info"));
			infPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

			ctlPane.Create(hSplit2.m_hWnd);
			ctlPane.SetTitle(_T("ctrl"));
			ctlPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

			dbgPane.Create(hSplit2.m_hWnd);
			dbgPane.SetTitle(_T("debug"));
			dbgPane.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON);

			hSplit1.SetSplitterPanes(infPane, hSplit2);
			hSplit1.SetSplitterPanes(ctlPane, dbgPane);

			mTcp.set_print(print);
			mMqtt.set_print(print);
		}

		void ctlPaneInit(void)
		{
			CRect rc;
			CFont font;
			

			//ctlPane.GetClientRect(&rc);

			rc.left = 20;
			rc.right = 100;
			rc.top = 40;
			rc.bottom = 80;
			btConn.Create(ctlPane.m_hWnd, rc, "connect", WS_VISIBLE | WS_CHILD, NULL, ID_CONN, NULL);

			rc.left = 120;
			rc.right = 200;
			btCali.Create(ctlPane.m_hWnd, rc, "cali start", WS_VISIBLE | WS_CHILD, NULL, ID_CALI, NULL);

			rc.left = 220;
			rc.right = 300;
			//btSav.Create(ctlPane.m_hWnd, rc, _T("save start"), WS_VISIBLE | WS_CHILD, NULL, ID_DATA_SAVE, NULL);
			btSav.Create(ctlPane.m_hWnd, rc, "trigger", WS_VISIBLE | WS_CHILD, NULL, ID_TRIGGER, NULL);

			rc.left = 20;
			rc.right = 100;
			//logClr.Create(ctlPane.m_hWnd, rc, _T("log clear"), WS_VISIBLE | WS_CHILD, NULL, ID_LOG_CLR, NULL);
		}

		void dbgPaneInit(void)
		{
			CRect rc;

			dbgPane.GetClientRect(&rc);

			debug.Create(dbgPane.m_hWnd, &rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
				WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN);
			debug.SetReadOnly(1);
			debug.LimitText(0x99999999);
			debug.ShowScrollBar(1);

			dbgPane.SetClient(debug.m_hWnd);
		}



		void paneInit(void)
		{
			ctlPaneInit();
			dbgPaneInit();
		}

		static void mqtt_msg_published(void* context, int dt, int packet_type, MQTTProperties* properties, enum MQTTReasonCodes reasonCode)
		{
			
		}
		static int mqtt_msg_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* m)
		{
			int i;
			//vector<char>mqttBuffer;

			print("recv: %s\n", (char*)m->payload);

			MQTTClient_freeMessage(&m);
			MQTTClient_free(topicName);

			return 1;
		}
		static void mqtt_disconn(void* context, MQTTProperties* properties, enum MQTTReasonCodes reasonCode)
		{
			mMqtt.conn(devMeta.hostName, devMeta.port, sign.clientId, sign.username, sign.password);
		}
		int startClient(void)
		{
			if (proto==0) {
				extern DWORD WINAPI sockRecvThread(LPVOID lpParam);
				char* ip =(char*) "192.168.2.88";
				mTcp.conn(ip, 8888, sockRecvThread);
			}
			else {
				mqtt_sign(&devMeta, &sign, SIGN_HMAC_SHA1);
				mMqtt.conn(devMeta.hostName, devMeta.port, sign.clientId, sign.username, sign.password);

				mqtt_callback_t cb;

				memset(&cb, 0, sizeof(cb));
				cb.arrived = mqtt_msg_arrived;
				cb.published = mqtt_msg_published;
				cb.disconn = mqtt_disconn;
				mMqtt.set_cb(&cb);
				mMqtt.subscribe(mqttTopic);

				//SetTimer(88, 1000, NULL);
			}

			return 0;
		}

		int stopClient(void)
		{
			if (proto == 0) {
				mTcp.disconn();
			}
			else {
				mMqtt.disconn();

				KillTimer(88);
			}

			return 0;
		}


		

		LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
		{
			splitInit();
			paneInit();
			
			return 0;
		}
		

		LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
		{
			

			return 0;
		}

		
		LRESULT OnBtnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
		{
			static int connFlag = 0;
			static int caliFlag = 0;

			CButton btn = hWndCtl;

			switch (wID) {
			case ID_CONN:
			{
				if (connFlag == 0) {
					startClient();
					connFlag = 1;
					btConn.SetWindowText("disconnect");
				}
				else {
					stopClient();
					connFlag = 0;
					btConn.SetWindowText("connect");
				}
			}
			break;

			case ID_CALI:
			{
				if (caliFlag==0) {

					btCali.SetWindowText("cali stop");
					caliFlag = 1;
				}
				else {

					btCali.SetWindowText("cali start");
					caliFlag = 0;
				}
			}
			break;

			case ID_LOG_CLR:
			{
				debug.SetWindowText(_T(""));
			}
			break;

			case ID_LOG_STOP:
			{
				if (printEnableFlag == 0) {


					printEnableFlag = 1;
				}
				else {


					printEnableFlag = 0;
				}
			}
			break;

			case ID_LOG_SAVE:
			{
				//extern void writeFile(U8 ch, void* data, int len);
			}
			break;

			case ID_DAT_SAVE:
			{
				
			}
			break;

			case ID_TRIGGER:
			{
				int r;
				char tmp[100];
				static int pub_cnt = 0;

				if (mMqtt.is_connected()) {
					memset(tmp, 0, sizeof(tmp));
					sprintf(tmp, "XXXX %d", pub_cnt++);

					print("pub: %s\n", tmp);
					r = mMqtt.publish(mqttTopic, tmp, 1);
				}
				else {
					print("___mqtt not connected!\n");
				}
			}
			break;



			}



			return 0;
		}

		int wait_ack_evt(void)
		{
			DWORD r;

			r = WaitForSingleObject(ackEvent, 500);
			if (r!=0) {
				return -1;
			}

			return 0;
		}
		static void set_ack_evt(void)
		{
			SetEvent(ackEvent);
		}



		LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
		{

			DestroyWindow();
			PostQuitMessage(0);

			return 0;
		}

		LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
		{
			CPaintDC dc(m_hWnd);
			CMemoryDC mdc(dc,dc.m_ps.rcPaint);
			


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

		


		void OnSize(UINT nType, CSize size)
		{
			RedrawWindow();
			if (nType != SIZE_MINIMIZED) {
				//m_splitterWnd1.SetColumnInfo(0, cx / 3, cx / 3);
				//m_splitterWnd2.SetRowInfo(0, cy / 2, cy / 2);
				//vSplit.RecalcLayout();
				//hSplit1.RecalcLayout();

				RECT rc;
				GetClientRect(&rc);

				//vSplit.SetWindowPos(NULL, &rc, SWP_NOZORDER | SWP_NOACTIVATE);
				
			}

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



myFrame::myFrame()
{
	ackEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
}


myFrame::~myFrame()
{
	stopClient();
	CloseHandle(ackEvent);
}































