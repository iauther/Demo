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
#include "cfg.h"
#include "list.h"
#include "dsp.h"


#ifndef OS_KERNEL
/*
  gd32f470内部资源：  240MHz, 3072KB flash, 768KB sram
                     3*12bit 2.6 MSPS ADCs, 2*12bit DACs
                     8*16bit general timers, 2*16-bit PWM timers,
                     2*32bit general timers, 2*16bit basic timers
                     6*SPIs, 3*I2Cs, 4*USARTs 4*UARTs, 2*I2Ss
                     2*CANs, 1*SDIO1*USBFS 1*USBHS, 1*ENET
*/

#define RX_CNT              4000
#define TX_CNT              10


#define EV_CNT              1000

#define CAP_FREQ            (1000*KHZ)
#define CAP_TIMES           10
#define ADC_CAP_POINTS      (RX_CNT*CAP_TIMES)

static int adc_cap_start=0;
static int adc_cap_points=0;
static int adc_cap_finished=0;
static int adc_ev_cnt=0;
static F32 adc_ev[ADC_CAP_POINTS/EV_CNT];
handle_t   adc_cap_list=NULL;
static int log_recv_callback(handle_t h, void *addr, U32 evt, void *data, int len)
{
    int  r=0;
    
    if(strstr((char*)data, "START")) {
        adc_cap_points = 0;
        adc_cap_finished = 0;
        ads9120_enable(1);
    }
    
    return r;
}
static void ads_data_callback(F32 *data, int cnt)
{
    int r,len=cnt*4;

    if(adc_cap_points<ADC_CAP_POINTS)
    {
        r = list_append(adc_cap_list, data, len);
        if(r==0) {
            adc_cap_points += cnt;
        }
    }
    else {
        ads9120_enable(0);
        adc_cap_finished = 1;
    }
}


typedef struct {
    U16  rx[RX_CNT];
    int  rxLen;
    
    U16  tx[TX_CNT];
    
    F32  vx[RX_CNT];
    U16  ox[RX_CNT];
}my_buf_t;

static my_buf_t mBuf;
static int ads_init(void)
{
    int r;
    list_cfg_t lc;
    ads9120_cfg_t ac;
    
    
    lc.max = CAP_TIMES;
    lc.mode = MODE_FIFO;
    adc_cap_list = list_init(&lc);
    
    ac.buf.rx.buf  = (U8*)mBuf.rx;
    ac.buf.rx.blen = sizeof(mBuf.rx);
    ac.buf.tx.buf  = (U8*)mBuf.tx;
    ac.buf.tx.blen = sizeof(mBuf.tx);
    ac.buf.vx.buf  = (U8*)mBuf.vx;
    ac.buf.vx.blen = sizeof(mBuf.vx);
    ac.buf.ox.buf  = (U8*)mBuf.ox;
    ac.buf.ox.blen = sizeof(mBuf.ox);
    ac.freq = CAP_FREQ;
    ac.callback = NULL;
    
    r = ads9120_init(&ac);
    
    return r;
}

U32 wake_time=0;
void test_main(void)
{
    int i,r,cnt=0;
    handle_t hdac;
    handle_t htmr;
    handle_t hfile;
    U16 data;
    U8 tmp[100],pwr_flag=0;
    static U32 tm1,tm2,tm;
    ecxxx_cfg_t ec;
    node_t node;
    F32 *f32;
    int nEv;
    
    log_set_callback(log_recv_callback);
    //rtcx_init();
    ads_init();
    
    //ecxxx_test();
    //sflash_test();
    //dal_nor_test();
    //upgrade_test();

#if 1
    wake_time = dal_get_tick_ms();
    while(1) {
        if(adc_cap_finished) {
            
            adc_ev_cnt = 0;
            while(1) {
                r = list_get(adc_cap_list, &node, 0);
                if(r) {
                    break;
                }
                
                f32 = (F32*)node.buf;
                
                nEv = node.dlen/(EV_CNT*4);
                
                for(i=0; i<nEv; i++) {
                    dsp_ev_calc(EV_RMS, f32+i*EV_CNT, EV_CNT, &adc_ev[adc_ev_cnt++]);
                }
                
                for(i=0; i<node.dlen/4; i++) {
                    LOGD("%.03f\n", f32[i]);
                }
                
                list_remove(adc_cap_list, 0);
            }
            
            for(i=0; i<adc_ev_cnt; i++) {
                LOGD("%.03f\n", adc_ev[i]);
            }
        }
        
        tm = dal_get_tick_ms()-wake_time;
        if(!pwr_flag && tm>=(10*1000)) {
            
            //LOGD("power down now ...\n");
            //power_off(10);
            //pwr_flag = 1;
            
            //rtcx_set_countdown(10);
            
            //power_set(POWER_MODE_STANDBY);
            //wake_time = dal_get_tick_ms();
        }
    }
#endif
    
}

#endif

