#ifndef __MYTCP_Hx__
#define __MYTCP_Hx__

#include <windows.h>


typedef void (*mytcp_print_t)(const char* format, ...);

class myTcp {
public:
    myTcp();
    ~myTcp();
    
    int set_print(mytcp_print_t fn);
    int conn(char *ip, int port, LPTHREAD_START_ROUTINE recvFunc);
    int disconn(void);
    
private:
    SOCKET sHost;
	HANDLE hThread;
	DWORD dwThreadId;
    mytcp_print_t printFunc;
};





#endif