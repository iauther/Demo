#include "notice.h"
#include "paras.h"

 
int notice_start(U8 dev, U8 level)
{
    if(dev&DEV_LED) {
        led_blink_start(&allNotice[level].led);
    }
    
    if(dev&DEV_BUZZER) {
        buzzer_start(&allNotice[level].buzzer);
    }
    
    return 0;
}


int notice_stop(U8 dev)
{
    if(dev&DEV_LED) {
        led_blink_stop();
    }
    
    if(dev&DEV_BUZZER) {
        buzzer_stop();
    }
    
    return 0;
}


