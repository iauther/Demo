#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "ui.h"

int log_en_flag = 1;
extern uiMultilineEntry* logEntry;
static char log_buf[1000];
int logx(const char *fmt,...)
{
	int i;
	va_list args;

	if (!logEntry) {
		return -1;
	}

	va_start(args, fmt);
	i = vsnprintf(log_buf, sizeof(log_buf), fmt, args);
	va_end(args);
	if (log_en_flag) {
		uiMultilineEntryAppend(logEntry, log_buf);
	}

	return 0;
}

int logx_clr(void)
{
	if (!logEntry) {
		return -1;
	}

	uiMultilineEntrySetText(logEntry, "");
	return 0;
}

int logx_en(int flag)
{
	if (!logEntry) {
		return -1;
	}

	log_en_flag = flag;
	return 0;
}

int logx_save(char* path)
{
	int i, j;
	FILE* fp;

	if (!logEntry) {
		return -1;
	}

	fp = fopen(path, "wt");
	if (!fp) {
		logx("%s open failed, check the path please!", path);
		return -1;
	}
	fprintf(fp, uiMultilineEntryText(logEntry));
	fclose(fp);

	return 0;
}


int logx_get(void)
{
	return log_en_flag;
}

