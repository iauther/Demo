#ifndef __MYCLOUD_Hx__
#define __MYCLOUD_Hx__

#include <windows.h>



class myCloud {
public:
    myCloud();
    ~myCloud();

    int init(void);
    int deinit(void);
    int conn(void);
    int disconn(void);
    int read(void *data, int len);
    int write(void *data, int len);
    
private:
    int flag;
};





#endif