#include "log.h"
#include "rs485.h"
#include "ecxxx.h"
#include "ads9120.h"
#include "dal_dac.h"
#include "dal_pwm.h"
#include "dal_uart.h"
#include "dal_delay.h"
#include "dal_timer.h"
#include "dal_nor.h"
#include "power.h"
#include "ecxxx.h"
#include "dal.h"
#include "rtc.h"
#include "sflash.h"
#include "upgrade.h"
#include "fs.h"


#ifndef OS_KERNEL

#define RECV_BUF_LEN   1000

typedef struct {
    buf_t  ecxxx;
    buf_t  rs485;
    buf_t  log;
}recv_buf_t;


static recv_buf_t recvBuf;
static void recv_buf_init(buf_t *t, int len)
{
    t->blen = len;
    t->dlen = 0;
    t->buf = malloc(RECV_BUF_LEN);
    if(t->buf) {
        t->blen = RECV_BUF_LEN;
    }
}
static void recvBuf_init(void)
{
    recv_buf_init(&recvBuf.ecxxx, RECV_BUF_LEN);
    recv_buf_init(&recvBuf.rs485, RECV_BUF_LEN);
    recv_buf_init(&recvBuf.log, RECV_BUF_LEN);
}

static int rs485_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    buf_t *tmp=&recvBuf.rs485;
    if(tmp->buf && (tmp->dlen==0) && len<=tmp->blen) {
        memcpy(tmp->buf, data, len);
        tmp->dlen = len;
    }
    
    return 0;
}
static int ecxxx_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    
    
    return 0;
}
static int log_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    buf_t *tmp=&recvBuf.log;
    if(tmp->buf && (tmp->dlen==0) && len<=tmp->blen) {
        memcpy(tmp->buf, data, len);
        tmp->dlen = len;
    
    }
    
    return 0;
}







////////////////////////////////////////////////
/*
  gd32f470内部资源：  240MHz, 3072KB flash, 768KB sram
                     3*12bit 2.6 MSPS ADCs, 2*12bit DACs
                     8*16bit general timers, 2*16-bit PWM timers,
                     2*32bit general timers, 2*16bit basic timers
                     6*SPIs, 3*I2Cs, 4*USARTs 4*UARTs, 2*I2Ss
                     2*CANs, 1*SDIO1*USBFS 1*USBHS, 1*ENET
*/

static void print_fw_info(char *s, fw_info_t *info)
{
    LOGD("\n");
    LOGD("__%s__fwInfo->magic:   0x%08x\n",    s, info->magic);
    LOGD("__%s__fwInfo->version: %s\n",        s, info->version);
    LOGD("__%s__fwInfo->bldtime: %s\n",        s, info->bldtime);
    LOGD("__%s__fwInfo->length:  %d\n",        s, info->length);
    LOGD("\n");
}


#define MOUNT_DIR      "/sd"
#define ONCE_LEN        (1024*4)
int upgrade_test(void)
{
    int flen;
    int r,rlen,index=0;
    U8 *pbuf=NULL;
    int upg_flag=0;
    char *app=MOUNT_DIR"/app.upg";
    handle_t fl,fs = fs_init(DEV_SDMMC, FS_FATFS);
    
    pbuf = (U8*)malloc(ONCE_LEN);
    if(!pbuf) {
        LOGE("____ upgrade test malloc failed\n");
        return -1;
    }
    
    r = fs_mount(fs, MOUNT_DIR);
    if(r) {
        LOGE("____ fs mount failed\n");
        goto fail;
    }
    
    //fs_scan(fs, MOUNT_DIR);
    
    
    fl = fs_open(fs, app, FS_MODE_RW);
    if(!fl) {
        LOGE("____ %s open failed\n", app);
        goto fail;
    }
    
    flen = fs_size(fl);
    if(flen<=0) {
        LOGE("____ upgrade file len wrong\n");
        fs_close(fl);
        goto fail;
    }
        
    LOGD("____ upgrade file len: %d\n", flen);
    
    while(1) {
        rlen = fs_read(fl, pbuf, ONCE_LEN);
        //LOGD("_____%d, 0x%02x, 0x%02x, \n", rlen, tmp[0], tmp[1]);
        if(rlen>0) {
            
            if(index==0) {
                fw_info_t *info1=(fw_info_t*)pbuf;
                fw_info_t info2;
                
                print_fw_info("111", info1);
                
                r = upgrade_get_fw_info(&info2);
                if(r==0) {
                    print_fw_info("222", &info2);
                    
                    if(memcmp(info1, &info2, sizeof(fw_info_t))==0) {
                        LOGD("____ fw is same, quit!\n");
                        break;
                    }
                }
            }
            
            if(rlen<ONCE_LEN) {
                r = upgrade_write(pbuf, rlen, -1);
                if(r) {
                    LOGE("____ upgrade failed\n");
                }
                else {
                    LOGD("____ upgrade ok\n");
                    upg_flag = 1;
                }
                break;
            }
            
            r = upgrade_write(pbuf, rlen, index++);
            if(r) {
                LOGE("____ upgrade write failed, index: %d\n", index);
                break;
            }
        }
        else {
            LOGE("____ fs_read error\n");
            break;
        }
    }
    fs_close(fl);
    
    if(upg_flag) {
        LOGD("____ reboot now ...\n\n\n");
        dal_reboot();
    }

fail:
    fs_deinit(fs);
    
    return r;
}




#define RX_LEN  1000
#define TX_LEN  10

typedef struct {
    U16  rx[RX_LEN];
    int  rxLen;
    
    
    U16  tx[TX_LEN];
    
    U16  ox[RX_LEN/DAC_FREQ_DIV];
}my_buf_t;

static my_buf_t mBuf;
static int ads_init(void)
{
    int r;
    ads9120_cfg_t ac;
    
    ac.buf.rx.buf  = (U8*)mBuf.rx;
    ac.buf.rx.blen = sizeof(mBuf.rx);
    ac.buf.tx.buf  = (U8*)mBuf.tx;
    ac.buf.tx.blen = sizeof(mBuf.tx);
    ac.buf.ox.buf  = (U8*)mBuf.ox;
    ac.buf.ox.blen = sizeof(mBuf.ox);
    ac.freq = 2500*KHZ;
    ac.callback = NULL;
    
    r = ads9120_init(&ac);
    
    return r;
}

U32 wake_time=0;
void test_main(void)
{
    int r,cnt=0;
    ads9120_cfg_t ac;
    dal_timer_cfg_t tc;
    handle_t hdac;
    handle_t htmr;
    handle_t hfile;
    U16 data;
    U8 tmp[100],pwr_flag=0;
    static U32 tm1,tm2,tm;
    ecxxx_cfg_t ec;
    
    
    ads_init();
    
    //ecxxx_test();
    //sflash_test();
    //dal_nor_test();
    //upgrade_test();

#if 0
    if(strcmp(VERSION, "V1.0.0")==0) {
        upgrade_test2();
        while(1) {
            dal_delay_ms(500);
        }
    }
    else {
        while(1) {
            LOGD("upgreade test %d\n", cnt++);
            dal_delay_ms(500);
        }
    }
#endif

#if 0
    wake_time = dal_get_tick_ms();
    while(1) {
        
        tm = dal_get_tick_ms()-wake_time;
        if(!pwr_flag && tm>=(SAMPLE_INV_TIME*1000)) {
            
            //LOGD("power down now ...\n");
            //power_off(10);
            //pwr_flag = 1;
            
            //power_set(POWER_MODE_STANDBY);
            //wake_time = dal_get_tick_ms();
        }
    }
#endif
    
}

#endif

