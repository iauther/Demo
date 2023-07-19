#include <WS2tcpip.h>
#include "myTcp.h"
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")



myTcp::myTcp()
{
    WSADATA wsaData;
    
	printFunc = (mytcp_print_t)printf;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		//printf("Winsock load faild!\n");
		return;
	}
}

myTcp::~myTcp()
{
    WSACleanup();
}


int myTcp::set_print(mytcp_print_t fn)
{
	printFunc = fn;
	return 0;
}


int myTcp::conn(char *ip, int port, LPTHREAD_START_ROUTINE recvFunc)
{
    SOCKADDR_IN servAddr;
	servAddr.sin_family = AF_INET;
	//  注意   当把客户端程序发到别人的电脑时 此处IP需改为服务器所在电脑的IP
	inet_pton(AF_INET, ip, (void*)&servAddr.sin_addr.S_un.S_addr);
	servAddr.sin_port = htons(port);

	sHost = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sHost == INVALID_SOCKET) {
		printFunc("socket faild!\n");
		return -1;
	}

	if (connect(sHost, (LPSOCKADDR)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
		printFunc("connect faild!\r\n");
		closesocket(sHost);
		return -1;
	}
	
	hThread = ::CreateThread(NULL, NULL, recvFunc, (LPVOID)&sHost, 0, &dwThreadId);
	if (!hThread) {
		printFunc("tcp client recv thread creat failed!\n");
		return -1;
	}
	printFunc("tcp client start ok!\n");
    
    return 0;
}


int myTcp::disconn(void)
{
    shutdown(sHost, SD_BOTH);
	closesocket(sHost);
	
	printFunc("tcp client socket closed\n");

	::WaitForSingleObject(hThread, INFINITE);
	::CloseHandle(hThread);
	hThread = NULL;
    
    return 0;
}







