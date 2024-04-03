
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
    TCHAR* replace(TCHAR* str, const TCHAR* src, const TCHAR* dst);
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
    TCHAR* ptxt;

    if (!inited || !en_flag || !lev[lv].enable) {
        return;
    }

#if 0
    //char2wchar(fmt, sizeof(fmt), (char*)format);
    va_start(list, format);
    
    sprintf(buffer, format, list);
    va_end(list);
#endif

    //֧��\r\n��\n����
    ptxt = replace(txt, _T("\n"), _T("\r\n"));
    if (!ptxt) {
        return;
    }

    //char2wchar(tmpBuf, sizeof(tmpBuf), tmp);
    //debug.AppendText(dbgBuf, 1, 0);
    //debug.SetText(dbgBuf);
    
    dbg.SetRedraw(FALSE);
    int txtLen = dbg.GetWindowTextLength();
    dbg.SetSel(txtLen, txtLen);      //�ƶ���굽���
    dbg.SetFocus();                             //�ƶ���굽���
    dbg.ReplaceSel(ptxt);                     //�ڹ���λ�ü������µ������־��
    dbg.LineScroll(dbg.GetLineCount());
    dbg.SetRedraw(TRUE);
    //free(ptxt);
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

TCHAR* myLog::replace(TCHAR* str, const TCHAR* src, const TCHAR* dst)
{
    int i,j=0;
    size_t len = _tcslen(str);
    size_t len1 = _tcslen(src);
    size_t len2 = _tcslen(dst);
    TCHAR* p = new TCHAR[len * 2];

    if (p) {
        for (i = 0; i < len;) {
            //�ҵ�src��û�ҵ�dst
            if (memcmp(str + i, src, len1)==0 && memcmp(str + i, dst, len2)) {
                memcpy(p + j, dst, len2);
                i += len1; j += len2;
            }
            else {
                p[j] = str[i];
                i++; j++;
            }
        }
        p[j] = 0;
    }
    
    return p;
}




//////////////////////////////////////////////////
static myLog mLog;
static HANDLE hMutex = NULL;
void log_init(HWND hwnd, CRect rc, CFont font)
{
    mLog.init(hwnd, rc, font);
    HANDLE hMutex = CreateMutex(nullptr, FALSE, NULL);
}

HWND log_get_hwnd(void)
{
    return mLog.get_hwnd();
}

void log_clear(void)
{
    WaitForSingleObject(hMutex, INFINITE);
    mLog.clear();
    ReleaseMutex(hMutex);
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

static TCHAR log_buff[10000];
void log_print(LOG_LEVEL lv, const char* format, ...)
{
    va_list args;

    WaitForSingleObject(hMutex, INFINITE);

    va_start(args, format);
    _vstprintf_s(log_buff, sizeof(log_buff), format, args);
    va_end(args);
    
    mLog.print(lv, log_buff);
    ReleaseMutex(hMutex);
}

int log_save(const char* path)
{
    int r;
    WaitForSingleObject(hMutex, INFINITE);
    r = mLog.save(path);
    ReleaseMutex(hMutex);

    return r;
}










