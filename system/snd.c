#include "snd.h"
#include "log.h"
#include "snd/pcm186x.h"
#include "snd/adau197x.h"


int snd_init(void)
{
    int r;
    
    r = pcm186x_init(PCM186X_ID_1);
    if(!r) {
        LOGE("___ PCM186X_ID_1 init failed\n");
    }
    
    r = pcm186x_init(PCM186X_ID_2);
    if(!r) {
        LOGE("___ PCM186X_ID_2 init failed\n");
    }
    
    r = adau197x_init(ADAU197X_ID_1);
    if(!r) {
        LOGE("___ ADAU197X_ID_1 init failed\n");
    }
    
    r = adau197x_init(ADAU197X_ID_2);
    if(!r) {
        LOGE("___ ADAU197X_ID_2 init failed\n");
    }
    
    return 0;
}


















