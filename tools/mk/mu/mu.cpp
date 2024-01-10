#include "myFile.h"
#include "protocol.h"
#include "datetime.h"
#include "md5.h"


TCHAR* cfgPath = (TCHAR*)__TEXT("./user/cfg.h");

static void print_fw(const char* s, fw_hdr_t* hdr)
{
	printf("\n");
	printf("__%s__hdr.fw.magic:     0x%08x\n", s, hdr->fw.magic);
	printf("__%s__hdr.fw.version:   %s\n", s, hdr->fw.version);
	printf("__%s__hdr.fw.length:    %d\n", s, hdr->fw.length);
	printf("__%s__hdr.fw.bldtime:   %s\n", s, hdr->fw.bldtime);

	printf("__%s__hdr.upg.obj:      %d\n", s, hdr->upg.obj);
	printf("__%s__hdr.upg.force:    %d\n", s, hdr->upg.force);
	printf("__%s__hdr.upg.erase:    %d\n", s, hdr->upg.erase);
	printf("__%s__hdr.upg.flag :    %d\n", s, hdr->upg.flag);
	printf("__%s__hdr.upg.jumpAddr: 0x%08x\n", s, hdr->upg.jumpAddr);
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
	fw_hdr_t hdr;
	char* path = argv[1];
	//char* goal = argv[1];
	TCHAR* xPath;
	myFile mf;

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

	hdr.upg.obj = OBJ_APP;
	hdr.upg.force = 0;
	hdr.upg.erase = 0;
	hdr.upg.flag = 0;
	hdr.upg.jumpAddr = 0;
	hdr.fw.magic = FW_MAGIC;
	hdr.fw.length = dat.dlen;
	mf.get_version(cfgPath, hdr.fw.version, sizeof(hdr.fw.version));
	mf.get_time(xPath, (char*)hdr.fw.bldtime, sizeof(hdr.fw.bldtime));
	print_fw("$$$", &hdr);

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

