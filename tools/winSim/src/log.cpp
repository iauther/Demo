#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "ui.h"

int log_en_flag = 1;
extern uiMultilineEntry* logEntry;
static char log_buf[1000];
int log(const char *fmt, ...)
{
	int i;
	va_list args;

	if (!logEntry) {
		return -1;
	}

	va_start(args, fmt);
	i = vsprintf(log_buf, fmt, args);
	va_end(args);
	if (log_en_flag) {
		uiMultilineEntryAppend(logEntry, log_buf);
	}

	return 0;
}

int log_clr(void)
{
	if (!logEntry) {
		return -1;
	}

	uiMultilineEntrySetText(logEntry, "");
	return 0;
}

int log_en(int flag)
{
	if (!logEntry) {
		return -1;
	}

	log_en_flag = flag;
}

int log_save(char* path)
{
	int i, j;
	FILE* fp;

	if (!logEntry) {
		return -1;
	}

	fp = fopen(path, "wt");
	if (!fp) {
		log("%s open failed, check the path please!", path);
		return -1;
	}
	fprintf(fp, uiMultilineEntryText(logEntry));
	fclose(fp);

	return 0;
}


int log_get(void)
{
	return log_en_flag;
}

