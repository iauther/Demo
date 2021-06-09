// mkfw.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "data.h"
#include "date.h"
#include "md5.h"
#include "cfg.h"


typedef struct {
	U8  *data;
	U32 dlen;
}data_t;

static long get_flen(FILE* fp)
{
	long len;
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	return len;
}
static int file_read(char* path, data_t* dat)
{
	U8* buf;
	FILE* fp;
	long  flen, rlen;

	fp = fopen(path, "rb");
	if (!fp) {
		printf("%s open failed\r\n", path);
		return -1;
	}
	flen = get_flen(fp);

	buf = (U8*)malloc(flen);
	if (!buf) {
		fclose(fp);
		return -1;
	}

	rlen = fread(buf, 1, flen, fp);
	dat->data = buf;
	dat->dlen = rlen;
	fclose(fp);

	return 0;
}

static int get_build_date(char *path, char *date)
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
	sprintf(date, "%04u%02u%02u", rtime.wYear, rtime.wMonth, rtime.wDay);

	return 0;
}



static fw_info_t fwInfo = {
	 FW_MAGIC,
	 VERSION,
	__DATE__,
};

int main(char argc, char *argv[])
{
	int r;
	FILE* fp;
	data_t dat;
	md5_t md5;
	char newPath[1000];
	upgrade_hdr_t hdr;
	char* path = argv[1];
    
	if(argc<2) {
		printf("input the firmware file, please!\n");
		return -1;
	}

	get_build_date(path, (char*)fwInfo.bldtime);
	hdr.upgInfo.fwInfo = fwInfo;
	hdr.upgInfo.force = 0;
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

	r = file_read(path, &dat);
	if (r) {
		return -1;
	}

	md5_calc(&hdr.upgCtl, sizeof(hdr)-sizeof(hdr.upgInfo), dat.data, dat.dlen, (char*)md5.digit);

	fwrite(&md5, 1, sizeof(md5), fp);
	fwrite(&hdr, 1, sizeof(hdr), fp);
	fwrite(dat.data, 1, dat.dlen, fp);
	free(dat.data);
	fclose(fp);

	return 0;
}

