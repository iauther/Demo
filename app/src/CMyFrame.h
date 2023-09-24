#pragma once

#include "wtl.h"

enum {
	ID_TEST=0x8000,


};


extern CAppModule appModule;
class CMyFrame : public CFrameWindowImpl<CMyFrame>, 
			public CUpdateUI<CMyFrame>,
			public CMessageFilter, 
			public CIdleHandler
{
public:
	CSplitterWindow m_wndSplitter;
	CPaneContainer  m_wndFolderTree;
	CTreeViewCtrlEx m_wndTreeView;
	CListViewCtrl m_wndListView;

	//DECLARE_FRAME_WND_CLASS(NULL, 500)

	BEGIN_UPDATE_UI_MAP(CMyFrame)
		UPDATE_ELEMENT(ID_TEST, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()


	BEGIN_MSG_MAP(CMyFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		COMMAND_HANDLER(ID_TEST, BN_CLICKED, OnBtnTest)

		REFLECT_NOTIFICATIONS()
		FORWARD_NOTIFICATIONS()

		CHAIN_MSG_MAP(CUpdateUI<CMyFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMyFrame>)
	END_MSG_MAP()


	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CFrameWindowImpl<CMyFrame>::PreTranslateMessage(pMsg);
	}

	virtual BOOL OnIdle()
	{
		return FALSE;
	}



	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CreateSimpleStatusBar();

		m_hWndClient = m_wndSplitter.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

		m_wndFolderTree.Create(m_wndSplitter, _T("Folders"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

		m_wndTreeView.Create(m_wndFolderTree, rcDefault, NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
			WS_EX_CLIENTEDGE);

		m_wndFolderTree.SetClient(m_wndTreeView);

		m_wndListView.Create(m_wndSplitter, rcDefault, NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS,
			WS_EX_CLIENTEDGE);
		m_wndListView.SetExtendedListViewStyle(LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_UNDERLINEHOT);

		InitViews();

		CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);


		InitViews();

		m_wndSplitter.SetSplitterPanes(m_wndFolderTree, m_wndListView);

		RECT rect;
		GetClientRect(&rect);
		m_wndSplitter.SetSplitterPos((rect.right - rect.left) / 4);

		UISetCheck(ID_TEST, 1);

		CRect rc;
		CButton bt;
		rc.left = 60;
		rc.right = rc.left + 60;
		rc.top = 60;
		rc.bottom = rc.top + 40;
		bt.Create(m_wndFolderTree, rc, "open", WS_CHILD | WS_VISIBLE, NULL, ID_FILE_NEW, NULL);

		CMessageLoop* pLoop = appModule.GetMessageLoop();
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		return 0;
	}

	LRESULT OnBtnTest(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		return 0;
	}

	void InitViews()
	{
		// Create list view columns
		m_wndListView.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 200, 0);
		m_wndListView.InsertColumn(1, _T("Size"), LVCFMT_RIGHT, 100, 1);
		m_wndListView.InsertColumn(2, _T("Type"), LVCFMT_LEFT, 100, 2);
		m_wndListView.InsertColumn(3, _T("Modified"), LVCFMT_LEFT, 100, 3);
		m_wndListView.InsertColumn(4, _T("Attributes"), LVCFMT_RIGHT, 100, 4);

	}
};



