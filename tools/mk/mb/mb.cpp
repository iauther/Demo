#include <windows.h>
#include <tchar.h>
#include <time.h>
#include "protocol.h"
#include "../projects/all/gd32f470/user/cfg.h"

//https://blog.csdn.net/phunxm/article/details/5082618
//https://blog.csdn.net/byxdaz/article/details/80507510


#define BOOT_SIZE		(48*1024)
#define INFO_SIZE		(16*1024)
#define APP_SIZE        (960*1024)

typedef struct {
	U8  *data;
	U32 dlen;
}data_t;

#pragma pack (1)
typedef struct {
	U8 boot[BOOT_SIZE];
	U8 info[INFO_SIZE];
	U8 app[APP_SIZE];
}fw_image_t;
#pragma pack ()

static TCHAR* char_to_tchar(char* str)
{
	DWORD dwNum = 0;
	dwNum = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	TCHAR* pUni = new TCHAR[dwNum];
	MultiByteToWideChar(CP_ACP, 0, str, -1, pUni, dwNum);
	return pUni;
}

static int file_read(const TCHAR* path, U8 *buf, int buflen)
{
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
		_tprintf(_T("%s open failed\n"), path);
		return -1;
	}

	flen = GetFileSize(hFile, NULL);
	if (flen== INVALID_FILE_SIZE) {
		printf("invalid file len\n");
		return -1;
	}

	if (flen> buflen) {
		printf("buf is too small!\n");
		return -1;
	}

	r = ReadFile(hFile, buf, flen, &rlen, NULL);
	if (r==FALSE) {
		printf("ReadFile failed\n");
	}

	CloseHandle(hFile);

	return flen;
}

static int get_time(TCHAR* path, char* time, int len)
{
	FILETIME IpCreationTime;                 //文件的创建时间
	FILETIME IpLastAccessTime;               //对文件的最近访问时间
	FILETIME IpLastWriteTime;                //文件的最近修改时间
	FILETIME ltime;
	SYSTEMTIME rtime;

	HANDLE hFile = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
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
	snprintf(time, len, "%04u%02u%02u_%02u%02u%02u", rtime.wYear, rtime.wMonth, rtime.wDay, rtime.wHour, rtime.wMinute, rtime.wSecond);

	return 0;
}
static void print_upg_data(const char* s, upg_data_t* upg)
{
	printf("\n");
	printf("__%s__upg->head:     0x%08x\n", s, upg->head);
	printf("__%s__upg->tail:     0x%08x\n", s, upg->tail);
	printf("__%s__upg->fwAddr:   0x%08x\n", s, upg->fwAddr);
	printf("__%s__upg->runAddr:  0x%08x\n", s, upg->runAddr);
	printf("\n");
}


int main(char argc, char *argv[])
{
	int r=0,flen,blanklen;
	HANDLE hFile=NULL;
	char* path1, * path2;
	char imgPath[500],*dot;
	fw_info_t fwInfo;
	char time1[100],time2[100],*ptime;
	TCHAR* xpath1=NULL, * xpath2 = NULL,* xpath3 = NULL;
	upg_data_t* pupg;
	fw_info_t *pfw;
	fw_image_t* img = (fw_image_t*)malloc(sizeof(fw_image_t));

	if (argc < 3) {
		printf("usage: mb xx/boot.bin yy/app.upg\n");
		return -1;
	}

	if (!img) {
		printf("iamge buf malloc failed\n");
		return -1;
	}

	memset(img, 0xff, sizeof(fw_image_t));

	path1 = argv[1];
	path2 = argv[2];

	xpath1 = char_to_tchar(path1);
	flen = file_read(xpath1, img->boot, BOOT_SIZE);
	if (flen <0) {
		goto fail;
	}

	xpath2 = char_to_tchar(path2);
	flen = file_read(xpath2, img->app, APP_SIZE);
	if (flen <0) {
		goto fail;
	}
	pfw = (fw_info_t*)img->app;
	blanklen = APP_SIZE - flen;

	pupg = (upg_data_t*)img->info;
	pupg->fwAddr = APP_OFFSET;
	pupg->runAddr = pupg->fwAddr + sizeof(fw_hdr_t);
	pupg->facFillFlag = 0;
	pupg->head = UPG_HEAD_MAGIC;
	pupg->tail = UPG_TAIL_MAGIC;
	pupg->upgFlag = UPG_FLAG_NORMAL;
	pupg->runFlag = 1;
	pupg->upgCnt = 0;

	print_upg_data("$$$", pupg);

	get_time(xpath1, time1, sizeof(time1));
	get_time(xpath2, time2, sizeof(time2));
	if (memcmp(time1, time2, sizeof(time1)) > 0) {
		ptime = time1;
	}
	else {
		ptime = time2;
	}

	strcpy(imgPath, path1);
	dot = strrchr(imgPath, '\\');
	if (dot) {
		dot[1] = 0;
	}
	else {
		imgPath[0] = 0;
	}

	strcat(imgPath, "image_");
	strcat(imgPath, (char*)pfw->version);
	strcat(imgPath, "_");
	strcat(imgPath, ptime);
	strcat(imgPath, ".bin");

	xpath3 = char_to_tchar(imgPath);
	hFile = CreateFile(xpath3,			//name
		GENERIC_WRITE,                  //以写方式打开
		0,                              //可共享读
		NULL,                           //默认安全设置
		CREATE_ALWAYS,                  //创建新文件
		FILE_ATTRIBUTE_NORMAL,          //常规文件属性
		NULL);
	if (!hFile) {
		goto fail;
	}
	WriteFile(hFile, img, sizeof(fw_image_t)- blanklen, NULL, NULL);

	printf(" %s created!!!\n", imgPath);

fail:
	if(hFile) CloseHandle(hFile);
	delete xpath1;delete xpath2;delete xpath3;
	free(img);

	return r;
}

