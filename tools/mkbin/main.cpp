// mkfw.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include "types.h"
#include "myCfg.h"

//https://blog.csdn.net/phunxm/article/details/5082618
//https://blog.csdn.net/byxdaz/article/details/80507510



typedef struct {
	U8  *data;
	U32 dlen;
}data_t;


#define BOOT_SIZE		(32*1024)


static int file_read(const TCHAR* path, data_t* dat)
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


static char tmpbuf[50 * 1000] = {0};
int main(char argc, char *argv[])
{
	int r;
	FILE* fp;
	data_t dat1,dat2;


	fp = fopen("burn.bin", "wb");
	if (!fp) {
		return -1;
	}

	r = file_read(L"boot.bin", &dat1);
	if (r) {
		return -1;
	}

	r = file_read(L"app.bin", &dat2);
	if (r) {
		return -1;
	}
	
	fwrite(dat1.data, 1, dat1.dlen, fp);
	fwrite(tmpbuf, 1, BOOT_SIZE-dat1.dlen, fp);
	fwrite(dat2.data, 1, dat2.dlen, fp);
	fclose(fp);

	free(dat1.data); free(dat2.data);

	return 0;
}

