#include "snd.h"
#include "log.h"
#include "cfg.h"
#include "snd/pcm186x.h"
#include "snd/adau197x.h"

//SAI: Serial Audio Interface
//访问SAI接口，需要用到BDMA，放在sai接口中调用

int snd_init(void)
{
    int r;
    
    r = pcm186x_init();
    if(r) {
        LOGE("___ PCM186X_ID_1 init failed\n");
    }

#ifdef USE_ADAU197X
    r = adau197x_init();
    if(r) {
        LOGE("___ ADAU197X_ID_1 init failed\n");
    }
#endif
    
    return 0;
}




