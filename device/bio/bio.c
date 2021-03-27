#include "log.h"
#include "drv/gpio.h"
#include "bio/bio.h"
#include "cfg.h"

volatile int bioDevice=-1;
static bio_handle_t *bio_handle=NULL;


extern int ad8233_probe(void);
extern bio_handle_t ad8233_handle;

#ifdef ADS129X_ENABLE
extern int ads129x_probe(void);
extern bio_handle_t ads129x_handle;
#endif

static int bio_comm_detect(void)
{
    int dev,r1,r2;
    
    r1 = ad8233_probe();
    
#ifdef ADS129X_ENABLE
    r2 = ads129x_probe();
    
    LOGI("_____r1: %d  r2: %d________\r\n", r1, r2);
    if(r1==0 && r2!=0) {
        dev = AD8233;
        LOGI("ad8233 device!\r\n");
    }
    else if(r1!=0 && r2==0) {
        dev = ADS129X;
        LOGI("ads129x device!\r\n");
    }
#else
    LOGI("_____r1: %d  ________\r\n", r1);
    if(r1==0) {
        dev = AD8233;
        LOGI("ad8233 device!\r\n");
    }
#endif
    else {
        dev = -1;
    }
    
    return dev;
}

//low: ad8233   1:ads129x
static int bio_io_detect(void)
{
    int dev;
    U8  hl;
    
    gpio_init(BIO_DETECT_PIN, HAL_GPIO_DIRECTION_INPUT, HAL_GPIO_DATA_HIGH);
    hl = gpio_get_hl(BIO_DETECT_PIN);
    dev = (hl==0)?AD8233:ADS129X;
    LOGI("_____bio device: %s\r\n", (dev==AD8233)?"AD8233":"ADS129X");
    
    return dev;
}



bio_handle_t *bio_probe(void)
{
    int r1,r2;
    
    if(bioDevice==-1) {
#ifdef BOARD_V00_01
        bioDevice = bio_comm_detect();
#else //BOARD_V00_02
        bioDevice = bio_io_detect();
#endif
    }
    
    if(bioDevice == AD8233) {
        return &ad8233_handle;
    }
#ifdef ADS129X_ENABLE
    else if(bioDevice == ADS129X) {
        return &ads129x_handle;
    }
#endif
    else {
        return NULL;
    }
}
