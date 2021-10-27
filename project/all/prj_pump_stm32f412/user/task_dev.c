#include "task.h"                          // CMSIS RTOS header file
#include "paras.h"
#include "data.h"
#include "wdog.h"
#include "pkt.h"

#define ACC0                  0.5F
#define ACC1                  5.0F        //accuracy, 1kpa

#define VMIN(v)             (v-ACC)
#define VMAX(v)             (v+ACC)


#define TIMER_MS       (100)           //ms

#define STAT_MS        (300)
#define STAT_CNT       (STAT_MS/TIMER_MS)


enum {
    PRES_RAISE=0,
    PRES_LEAK,
    PRES_KEEP,
};



static U16 prevSpeed=0;
static ms4525_t ms={0};
static handle_t mq=NULL;


static void auto_adjust(F32 dpres)
{
    U16 speed=0;
    sett_t *set=&curPara.setts.sett[curPara.setts.mode];
    F32 setPres=set->pres;
    F32 curPres=dpres;
    
#if 1    
    if(curPres<setPres) {
        valve_set(VALVE_CLOSE);
        
        if(air_act==AIR_LEAK) {
            vacuum_reached = 1;
        }
        
        if(vacuum_reached) {
            if(curPres>=setPres-ACC0) {
                air_act = AIR_KEEP;
                speed = 0;
            }
            else if(curPres<setPres-ACC0 && curPres<setPres-ACC1) {   //ษýัน
                speed = N950_SPEED_MIN;
            }
            else {
                speed = N950_SPEED_MAX;
            }
        }
        else {
            if(curPres>=setPres-ACC0) {
                air_act = AIR_KEEP;
                speed = N950_SPEED_MIN;
            }
            else {
                speed = N950_SPEED_MAX;
            }
        }
    }
    else {
        if(air_act==AIR_PUMP) {
            vacuum_reached = 1;
        }
        
        speed = 0;
        if(curPres<=setPres+ACC0) {  //ฝตัน
            air_act = AIR_KEEP;
            valve_set(VALVE_CLOSE);
        }
        else {
            valve_set(VALVE_OPEN);
        }
    }
#endif
    
    //the fllowing add for test
#if 0
    if(curPres>90.0F) {
        speed = 0;
        valve_set(VALVE_OPEN);
    }
    else if(curPres<30.0F) {
        speed = N950_SPEED_MAX;
        valve_set(VALVE_CLOSE);
    }
#endif
    
    if(speed!=prevSpeed) {
        n950_set_speed(speed);
        prevSpeed = speed;
    }
}


static int stat_cmp(stat_t *prev, stat_t *now)
{
    int r=0;
    
    if(ABS(prev->dPres-now->dPres)>0.99F) {
        r = 1;
    }
    *prev = *now;
    
    return r;
}

static int get_stat(stat_t *st)
{
    int r;
    F32 x;
    bmp280_t bmp;
    stat_t tmpStat={0};
    
    r = bmp280_get(&bmp);
    n950_get(&st->pump);
    
    st->adjMode  = adjMode;
    st->sysState = sysState;
    st->dPres = ms.pres;
    if(r==0 && bmp.pres<101.F) {
        x = ABS(bmp.pres-st->aPres);
        if(st->aPres>0.1F && x>1.0F) {
            ;
        }
        else {
            st->aPres = bmp.pres;
        }
        
        st->temp  = bmp.temp;
    }
    
    return 0;
}


static void dev_tmr_callback(void *arg)
{
    task_msg_post(TASK_DEV, EVT_TIMER, 0, NULL, 0);
}


static U8  color=GREEN;
static U32 tmr_cnt=0;
static void timer_proc(evt_t *e)
{
    wdog_feed();
    
    get_stat(&curStat);
    if(sysState==STAT_POWEROFF) {
        paras_set_state(STAT_STOP);
        if(curStat.aPres>0 && curStat.dPres>0 && (curStat.dPres<2.0F)) {
            valve_set(VALVE_CLOSE);
        }
    }
    else if(sysState!=STAT_UPGRADE) {
        paras_set_state(sysState);
    }
    
#if 0   //for test
    tmr_cnt++;
    if(tmr_cnt%STAT_CNT==0) {
        pkt_send(TYPE_STAT, 0, &curStat, sizeof(curStat));
        
        color = (color==GREEN)?BLUE:GREEN;
        led_set_color(color);
    }
#endif
}
static void valve_adjust(void)
{
    int r=1;
    ms4525_t m;
    
    r = ms4525_get(&m, 1);
    if(r==0) {

        if(curStat.aPres>0 && m.pres>curStat.aPres) {
            r++;
            return;
        }
        
        if(m.pres>0 && ms.pres>0 && ABS(ms.pres-ms.pres)>1.0F) {
            r++;
            return;
        }
        
        if(sysState==STAT_RUNNING && adjMode==ADJ_AUTO) {
            auto_adjust(ms.pres);
        }
        
        ms = m;
    }
}



static void task_valve(void *arg)
{
    while(1) {
        valve_adjust();
        delay_ms(20);
    }
}


void task_dev_fn(void *arg)
{
    int r;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    tmrId = osTimerNew(dev_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
    
    task_new(task_valve, 512);
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            
            switch(e.evt) {
                case EVT_TIMER:
                {
                    timer_proc(&e);
                }
                break;
                
                default:
                break;
            }
        }
        
    }
}



