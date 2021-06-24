// mkfw.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include "data.h"
#include "date.h"
#include "md5.h"

//https://blog.csdn.net/phunxm/article/details/5082618
//https://blog.csdn.net/byxdaz/article/details/80507510



typedef struct {
	U8  *data;
	U32 dlen;
}data_t;


static int file_read(TCHAR* path, data_t* dat)
{
	U8* buf;
	HANDLE hFile;
	DWORD  flen,rlen;
	BOOL r;

	hFile = CreateFile(path,			//name
		GENERIC_READ,                   //以读方式打开
		FILE_SHARE_READ,               //可共享读
		NULL,                           //默认安全设置
		OPEN_EXISTING,                   //只打开已经存在的文件
		FILE_ATTRIBUTE_NORMAL,           //常规文件属性
		NULL);                           //无模板
	if (hFile== INVALID_HANDLE_VALUE) {
		printf("%s open failed\r\n", path);
		return -1;
	}

	flen = GetFileSize(hFile, NULL);
	if (flen== INVALID_FILE_SIZE) {
		return -1;
	}

	buf = (U8*)malloc(flen);
	if (!buf) {
		CloseHandle(hFile);
		return -1;
	}

	r = ReadFile(hFile, buf, flen, &rlen, NULL);
	if (r==FALSE) {
		CloseHandle(hFile);
		return -1;
	}

	dat->data = buf;
	dat->dlen = rlen;
	CloseHandle(hFile);

	return 0;
}

static int get_buildtime(char *path, char *time, int len)
{
	FILETIME IpCreationTime;                 //文件的创建时间
	FILETIME IpLastAccessTime;               //对文件的最近访问时间
	FILETIME IpLastWriteTime;                //文件的最近修改时间
	FILETIME ltime;
	SYSTEMTIME rtime;

	HANDLE hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Get the dictionary failed!\n");
		CloseHandle(hFile);
		return -1;
	}
	if (!GetFileTime(hFile, &IpCreationTime, &IpLastAccessTime, &IpLastWriteTime)) {
		return -1;
	}
	CloseHandle(hFile);

	FileTimeToLocalFileTime(&IpLastWriteTime, &ltime);
	FileTimeToSystemTime(&ltime, &rtime);        //将文件时间转化为系统时间
	snprintf(time, len, "%04u%02u%02u", rtime.wYear, rtime.wMonth, rtime.wDay);

	return 0;
}


const PCTSTR path1 = __TEXT("\\user\\myCfg.h");
//const PCTSTR path1 = __TEXT("\\..\\..\\myCfg.h");
const PCTSTR path2 = __TEXT("\\..\\..\\project\\all\\prj_pump_stm32f412\\user\\myCfg.h");
static int get_version(void* version, int len)
{
	int r=-1;
	PTSTR ptr;
	char *p,*p1,*p2;
	data_t dat;
	TCHAR exePath[MAX_PATH];
	TCHAR fullPath[MAX_PATH];

	if (!version) {
		return -1;
	}
	  
	GetModuleFileName(NULL, exePath, MAX_PATH);
	ptr = _tcsrchr(exePath, L'\\');
	if (!ptr) {
		return -1;
	}
	*ptr = 0;

	_tcscpy(fullPath, exePath);_tcscat(fullPath, path1);
	r = file_read(fullPath, &dat);
	if (r) {
		_tcscpy(fullPath, exePath); _tcscat(fullPath, path2);
		r = file_read(fullPath, &dat);
		if (r) {
			return -1;
		}
	}

	p = strstr((char*)dat.data, "VERSION");
	if (p) {
		p1 = strchr(p, L'"');
		if (p1) {
			p1++;
			p2 = strchr(p1, '"');
			if (p2) {
				*p2 = 0;
				strcpy_s((char*)version, len, p1);
				r = 0;
			}
		}
	}

	free(dat.data);

	return r;
}


static TCHAR* ascii_to_unicode(char* str)
{
	DWORD dwNum = 0;
	dwNum = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	TCHAR * pUni = new TCHAR[dwNum];
	MultiByteToWideChar(CP_ACP, 0, str, -1, pUni, dwNum);
	return pUni;
}

int main(char argc, char *argv[])
{
	int r;
	FILE* fp;
	data_t dat;
	md5_t md5;
	char newPath[1000];
	upgrade_hdr_t hdr;
	fw_info_t fwInfo;
	char* path = argv[1];
	TCHAR* xPath;
    
	if(argc<2) {
		printf("input the firmware file, please!\n");
		return -1;
	}

	xPath = ascii_to_unicode(path);
	if (!xPath) {
		printf("ascii to unicode failed\n");
		return -1;
	}

	fwInfo.magic = FW_MAGIC;
	get_version(fwInfo.version, sizeof(fwInfo.version));
	get_buildtime(path, (char*)fwInfo.bldtime, sizeof(fwInfo.bldtime));
	hdr.fwInfo = fwInfo;
	hdr.upgCtl.force = 0;
	hdr.upgCtl.erase  = 0;
	hdr.upgCtl.action = 1;

	strcpy(newPath, path);
	char* dot = strrchr(newPath, '.');
	if (dot) *(dot + 1) = 0;
	strcat(newPath, "upg");

	fp = fopen(newPath, "wb");
	if (!fp) {
		printf("%s create failed\r\n", newPath);
		return -1;
	}

	r = file_read(xPath, &dat);
	if (r) {
		return -1;
	}

	md5_calc(&hdr.upgCtl, sizeof(hdr)-sizeof(hdr.fwInfo), dat.data, dat.dlen, (char*)md5.digit);

	fwrite(&md5, 1, sizeof(md5), fp);
	fwrite(&hdr, 1, sizeof(hdr), fp);
	fwrite(dat.data, 1, dat.dlen, fp);
	free(dat.data);
	fclose(fp);
	delete xPath;

	return 0;
}

