#include "myFile.h"
#include "protocol.h"
#include "datetime.h"


int myFile::read_file(TCHAR* path, data_t* dat)
{
	U8* pbuf;
	HANDLE hFile;
	DWORD  flen,rlen;
	BOOL r;

	hFile = CreateFile(path,			//name
		GENERIC_READ,                   //�Զ���ʽ��
		FILE_SHARE_READ,               //�ɹ����
		NULL,                           //Ĭ�ϰ�ȫ����
		OPEN_EXISTING,                   //ֻ���Ѿ����ڵ��ļ�
		FILE_ATTRIBUTE_NORMAL,           //�����ļ�����
		NULL);                           //��ģ��
	if (hFile== INVALID_HANDLE_VALUE) {
		_tprintf(_T("%ls open failed\n"), path);
		return -1;
	}

	flen = GetFileSize(hFile, NULL);
	if (flen== INVALID_FILE_SIZE) {
		_tprintf(_T("%s, get invalid file size!\n"), path);
		return -1;
	}

	pbuf = (U8*)malloc(flen);
	if (!pbuf) {
		printf("malloc %d failed!\n", flen);
		CloseHandle(hFile);
		return -1;
	}

	r = ReadFile(hFile, pbuf, flen, &rlen, NULL);
	if (r==FALSE) {
		printf("ReadFile failed!\n");
		CloseHandle(hFile);
		return -1;
	}

	dat->data = pbuf;
	dat->dlen = rlen;
	CloseHandle(hFile);

	return 0;
}

int myFile::get_time(TCHAR *path, char *time, int len)
{
	FILETIME IpCreationTime;                 //�ļ��Ĵ���ʱ��
	FILETIME IpLastAccessTime;               //���ļ����������ʱ��
	FILETIME IpLastWriteTime;                //�ļ�������޸�ʱ��
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
	FileTimeToSystemTime(&ltime, &rtime);        //���ļ�ʱ��ת��Ϊϵͳʱ��
	snprintf(time, len, "%04u%02u%02u %02u:%02u:%02u", rtime.wYear, rtime.wMonth, rtime.wDay, rtime.wHour, rtime.wMinute, rtime.wSecond);

	return 0;
}


int myFile::get_version(TCHAR* path, void* version, int len)
{
	int r=-1;
	PTSTR ptr;
	char *p,*p1,*p2;
	data_t dat;

	if (!version) {
		return -1;
	}
	
	r = read_file(path, &dat);
	if (r) {
	    return -1;
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


char* myFile::tchar_to_char(TCHAR* tchr)
{
	int clen = WideCharToMultiByte(CP_ACP, 0, tchr, -1, NULL, 0, NULL, NULL);
	char* chr = new char[clen];
	WideCharToMultiByte(CP_ACP, 0, tchr, -1, chr, clen, NULL, NULL);

	return chr;
}

TCHAR* myFile::char_to_tchar(char* chr)
{
	int clen = MultiByteToWideChar(CP_ACP, 0, chr, -1, NULL, 0);
	TCHAR* tchr = new TCHAR[clen];
	MultiByteToWideChar(CP_ACP, 0, chr, -1, tchr, clen);
	return tchr;
}


