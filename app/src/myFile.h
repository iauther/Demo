#ifndef __MYFILE_Hx__
#define __MYFILE_Hx__

#include <windows.h>

#define CHMAX				20
#define REC_THRESHOLD_LEN   500000


typedef struct {
	FILE* handle;
	int   flength;
}file_data_t;


class myFile {
public:
    myFile();
    ~myFile();

    void rec_start(int ch);
    void rec_stop(int ch);
    void stop_all(void);
    void rec_file(int ch, void* data, int len);
    
private:
    int         saveFlag;
    file_data_t recFile[CHMAX];
};





#endif