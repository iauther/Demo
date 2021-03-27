// mkfw.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include <stdlib.h>
#include "data.h"
#include "md5.h"

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


int main(char argc, char *argv[])
{
	int r;
	FILE* fp;
	data_t dat;
	char md5[40];
	char newPath[1000];
	upgrade_file_hdr_t hdr;
	char* path = argv[1];
    
	if(argc<2) {
		printf("input the firmware file, please!\n");
		return -1;
	}

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

	md5_calc((char*)dat.data, dat.dlen, (char*)hdr.md5);
	strcpy((char*)hdr.fwInfo.bldtime,  __DATE__);
	fwrite(&hdr, 1, sizeof(hdr), fp);
	fwrite(dat.data, dat.dlen, sizeof(hdr), fp);
	free(dat.data);
	fclose(fp);
	

	return 0;
}

