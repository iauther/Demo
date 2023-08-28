#ifndef __MYWAVE_Hx__
#define __MYWAVE_Hx__

#include "plplot.h"
#include "GuiLite.h"


class myWave {
public:
    myWave();
    ~myWave();
    
    int init(void);
    int update(void);
    
private:
    plstream *pls;
    PLINT    chartId;
    PLINT    errCode;
};





#endif