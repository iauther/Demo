
#include "myLog.h"

#define LOG_BUF_LEN   1000


class myLog
{
public:
    myLog();
    ~myLog();


    void init(HWND hwnd, CRect rc, CFont font);
    void enable(int flag);
    int  is_enable(void);
    HWND get_hwnd(void);
    void clear(void);
    int save(const char* path);
    void print(LOG_LEVEL lv, TCHAR* txt);
    void set_level(LOG_LEVEL lv, level_t * ld);
    

private:
    CEdit dbg;
    int   en_flag;
    TCHAR buffer[LOG_BUF_LEN];
    level_t lev[LV_MAX];
    int   inited;

    int char2wchar(wchar_t* wchar, int wlen, char* cchar);
    int is_tailof(TCHAR* src, TCHAR* str);
};


myLog::myLog()
{
    inited = 0;
    en_flag = 1;

    lev[LV_INFO].enable = 1;
    lev[LV_INFO].color = 0;

    lev[LV_DEBUG].enable = 1;
    lev[LV_DEBUG].color = 0;

    lev[LV_WARN].enable = 1;
    lev[LV_WARN].color = 0;

    lev[LV_ERROR].enable = 1;
    lev[LV_ERROR].color = 0;
}
myLog::~myLog()
{

}

void myLog::init(HWND hwnd, CRect rc, CFont font)
{
    dbg.Create(hwnd, rc, NULL, WS_CHILD | ES_LEFT | ES_NOHIDESEL | WS_TABSTOP | ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL);
    
	dbg.SetReadOnly(TRUE);
	dbg.LimitText(MAXDWORD);
	dbg.ShowScrollBar(SB_VERT);
    dbg.SetFont(font);
    inited = 1;
}

int myLog::save(const char *path)
{
    FILE* fp = fopen(path, "wt");
    if (!fp) {
        LOGE("___ fopen %s failed\n", path);
        return -1;
    }

    int len = dbg.GetWindowTextLength();
    char* buf = new char[len];
    if (buf) {
        dbg.GetWindowText(buf, len);
        fwrite(buf, 1, len, fp);
        delete[] buf;
    }
    fclose(fp);

    return 0;
}

void myLog::print(LOG_LEVEL lv, TCHAR *txt)
{
    int n = 0;
    va_list list;
    TCHAR fmt[200];
    SCROLLINFO ScroInfo;

    if (!inited || !en_flag || !lev[lv].enable) {
        return;
    }

#if 0
    //char2wchar(fmt, sizeof(fmt), (char*)format);

    va_start(list, format);
    //
    sprintf(buffer, format, list);
    va_end(list);
#endif

    //支持\r\n和\n换行
    if (is_tailof(txt, (char*)"\n")) {
        TCHAR* x = _tcsrchr(txt, '\n');
        if (x) _tcscpy(x, "\r\n");
    }

    //char2wchar(tmpBuf, sizeof(tmpBuf), tmp);
    //debug.AppendText(dbgBuf, 1, 0);
    //debug.SetText(dbgBuf);
    
    dbg.SetRedraw(FALSE);
    int txtLen = dbg.GetWindowTextLength();
    dbg.SetSel(txtLen, txtLen);      //移动光标到最后
    dbg.SetFocus();                             //移动光标到最后
    dbg.ReplaceSel(txt);                     //在光标的位置加入最新的输出日志行
    dbg.LineScroll(dbg.GetLineCount());
    dbg.SetRedraw(TRUE);

}

void myLog::set_level(LOG_LEVEL lv, level_t* ld)
{
    if (ld) {
        lev[lv] = *ld;
    }
}


void myLog::clear(void)
{
    dbg.SetWindowText("");
}

int myLog::is_enable(void)
{
    return en_flag;
}

void myLog::enable(int flag)
{
    en_flag = flag;
}

HWND myLog::get_hwnd(void)
{
    return dbg.m_hWnd;
}


int myLog::char2wchar(wchar_t* wchar, int wlen, char* cchar)
{
    int xlen = MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), NULL, 0);
    if (xlen >= wlen - 1) {
        xlen = wlen - 1;
    }
    MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), wchar, xlen);
    wchar[xlen] = '\0';

    return 0;
}

int myLog::is_tailof(TCHAR* src, TCHAR* str)
{
    int len1 = _tcslen(src);
    int len2 = _tcslen(str);
    int cmp = _tcscmp(src + len1 - len2, str);

    return (cmp == 0) ? 1 : 0;
}


//////////////////////////////////////////////////
static myLog mLog;
void log_init(HWND hwnd, CRect rc, CFont font)
{
    mLog.init(hwnd, rc, font);
}

HWND log_get_hwnd(void)
{
    return mLog.get_hwnd();
}

void log_clear(void)
{
    mLog.clear();
}

void log_enable(int flag)
{
    mLog.enable(flag);
}

int log_is_enable(void)
{
    return mLog.is_enable();
}

void log_set_level(LOG_LEVEL lv, level_t* ld)
{
    mLog.set_level(lv, ld);
}

void log_print(LOG_LEVEL lv, const char* format, ...)
{
    va_list args;
    TCHAR buff[1000];

    va_start(args, format);
    _vstprintf_s(buff, sizeof(buff), format, args);
    va_end(args);
    
    mLog.print(lv, buff);
}

int log_save(const char* path)
{
    return mLog.save(path);
}










