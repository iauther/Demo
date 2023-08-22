#include "myFile.h"
#include "protocol.h"
#include "date.h"
#include "md5.h"


TCHAR* cfgPath = (TCHAR*)__TEXT("./user/cfg.h");

static void print_fw_info(const char* s, fw_info_t* info)
{
	printf("\n");
	printf("__%s__fwInfo->magic:   0x%08x\n", s, info->magic);
	printf("__%s__fwInfo->version: %s\n", s, info->version);
	printf("__%s__fwInfo->bldtime: %s\n", s, info->bldtime);
	printf("__%s__fwInfo->length:  %d\n", s, info->length);
	printf("\n");
}
static void print_md5(const char* s, md5_t* m)
{
	U8 i;

	printf("__%s", s);
	for (i = 0; i < 32; i++) {
		printf("%c", m->digit[i]);
	}
	printf("\n");
}

int main(char argc, char *argv[])
{
	int r;
	FILE* fp;
	data_t dat;
	md5_t md5;
	char newPath[1000];
	upg_hdr_t hdr;
	char* path = argv[1];
	//char* goal = argv[1];
	TCHAR* xPath;
	myFile mf;
	upg_ctl_t* upg=&hdr.upgCtl;
	fw_info_t* info=&hdr.fwInfo;

	//TCHAR curPath[500];
	//GetCurrentDirectory(500, curPath);
	//_tprintf(_T("curPath: %s\n"), curPath);
    
	if(argc<2) {
		printf("usage: mu.exe app.bin\n");
		return -1;
	}

	xPath = mf.char_to_tchar(path);
	if (!xPath) {
		printf("char_to_tchar failed\n");
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

	r = mf.read_file(xPath, &dat);
	if (r) {
		return -1;
	}

	upg->obj = OBJ_APP;
	upg->force = 0;
	upg->erase = 0;
	info->magic = FW_MAGIC;
	info->length = sizeof(hdr) + dat.dlen;
	mf.get_version(cfgPath, info->version, sizeof(info->version));
	mf.get_time(xPath, (char*)info->bldtime, sizeof(info->bldtime));
	print_fw_info("$$$", info);

	md5_calc(&hdr, sizeof(hdr), dat.data, dat.dlen, (char*)md5.digit);
	fwrite(&hdr, 1, sizeof(hdr), fp);
	fwrite(dat.data, 1, dat.dlen, fp);
	fwrite(&md5, 1, sizeof(md5), fp);
	print_md5("md5: ", &md5);

	fclose(fp);
	free(dat.data);
	delete xPath;

	return 0;
}

