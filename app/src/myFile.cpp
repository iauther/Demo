#include <stdio.h>
#include "myFile.h"
#include "log.h"

#pragma comment(lib, "lib/hv_static.lib")


myFile::myFile()
{
    saveFlag = 0;
}

myFile::~myFile()
{

}

static void get_datatime(int ch, char* path, int len)
{
	SYSTEMTIME stime;
	GetLocalTime(&stime);
	snprintf(path, len, "rec_ch%02d_%04d%02d%02d_%02d%02d%02d.csv", ch, stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond);
}

void myFile::rec_start(int ch)
{
	char path[100];
	file_data_t* fd = NULL;
	
	if (ch >= CHMAX) {
		LOGE("___ ch %d is invalid, max: %d\n", ch, CHMAX);
		return;
	}

	fd = &recFile[ch];
	if (!fd->handle) {
		get_datatime(ch, path, sizeof(path));
		fd->handle = fopen(path, "w+");
		fd->flength = 0;
	}
}


void myFile::rec_stop(int ch)
{
	file_data_t* fd = NULL;

	if (ch >= CHMAX) {
		LOGE("___ ch %d is invalid, max: %d\n", ch, CHMAX);
		return;
	}

	if (!recFile[ch].handle) {
		LOGE("___ ch %d not open!\n", ch);
		return;
	}
	fd = &recFile[ch];

	fd->handle = NULL;
	fd->flength = 0;
}


void myFile::stop_all(void)
{
	int i;
	file_data_t* fd = NULL;

	for (i = 0; i < CHMAX; i++) {
		fd = &recFile[i];
		if (fd->handle) {
			fclose(fd->handle);
			fd->handle = NULL;
			fd->flength = 0;
		}
	}
}


void myFile::rec_file(int ch, void* data, int len)
{
	int i;
	uint32_t* p32 = (uint32_t*)data;
	file_data_t* fd = NULL;

	if (ch >= CHMAX || !data || !len) {
		LOGE("___ invalid param: ch: %d, data: %x, len: %d\n", ch, (uint32_t)data, len);
		return;
	}

	if (!saveFlag) {
		return;
	}

	fd = &recFile[ch];
	rec_start(ch);
	if (fd->handle && fd->flength < REC_THRESHOLD_LEN) {
		for (i = 0; i < len / 4; i++) {
			fprintf(recFile[ch].handle, "0x%x, \n", p32[i]);
		}
		fd->flength += len;
	}
}
