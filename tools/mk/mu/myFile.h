#include <windows.h>
#include <tchar.h>
#include <time.h>
#include "types.h"


typedef struct {
	U8  *data;
	U32 dlen;
}data_t;


class myFile {

public:
    char* tchar_to_char(TCHAR* tchr);
    TCHAR* char_to_tchar(char* chr);

    int read_file(TCHAR* path, data_t* dat);
    int get_time(TCHAR *path, char *time, int len);
    int get_version(TCHAR* path, void* version, int len);
    
};




