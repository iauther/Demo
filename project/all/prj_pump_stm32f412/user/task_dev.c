#include "task.h"                          // CMSIS RTOS header file
#include "paras.h"
#include "data.h" 
#include "pkt.h"


#define ACC                 1.0F        //accuracy, 1kpa

#define VMIN(v)             (v-ACC)
#define VMAX(v)             (v+ACC)


#define TIMER_MS            (100)           //ms

#define PARA_STAT_MS        (300)
#define PARA_STAT_CNT       (PARA_STAT_MS/TIMER_MS)


enum {
    PRES_RAISE=0,
    PRES_LEAK,
    PRES_KEEP,
};



static U8 presMode=PRES_RAISE;
static U8 vacuum_ever_reached=0;
static U16 prevSpeed=0;


static void auto_pres(stat_t *st)
{
    U16 speed=0;
    sett_t *set=&curPara.setts.sett[curPara.setts.mode];
    F32 setPres=set->pres;
    F32 curPres=st->dPres;
    
    if(presMode==PRES_LEAK) {
        if(curPres<setPres) {
            presMode = PRES_KEEP;
        }
    }
    else {
        if(curPres<setPres-ACC) {
            presMode = PRES_RAISE;
        }
        else if(curPres>setPres+ACC) {
            presMode = PRES_LEAK;
        }
        else {
            presMode = PRES_KEEP;
        }
    }
    
    switch(presMode) {
        
        case PRES_RAISE:
        {
            speed = N950_SPEED_MAX;
            vacuum_ever_reached = 0;
            valve_set(VALVE_CLOSE);
        }
        break;
        
        case PRES_LEAK:
        {
            speed = 0;
            valve_set(VALVE_OPEN);
        }
        break;
        
        case PRES_KEEP:
        {
            valve_set(VALVE_CLOSE);
            
            if(curPres>setPres) {
                speed = 0;
                vacuum_ever_reached = 1;
            }
            else if(setPres-ACC<curPres && curPres<setPres) {
                speed = N950_SPEED_MIN;
            }
            
            if(!vacuum_ever_reached) {
                speed = N950_SPEED_MAX;
            }
        }
        break;
    }
    
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
    ms4525_t ms;
    bmp280_t bmp;
    static U32 cnt=0;
    stat_t tmpStat={0};
    
    ms4525_get(&ms);
    bmp280_get(&bmp);
    n950_get(&st->pump);
    
    st->adjMode  = adjMode;
    st->sysState = sysState;
    st->dPres = ABS(ms.pres);
    st->aPres = bmp.pres;
    st->temp  = bmp.temp;

#if 0    
    cnt++;
    if(cnt%STAT_CNT==0 && stat_cmp(&tmpStat, st)) {
        pkt_send(TYPE_STAT, 0, st, sizeof(stat_t));
    }
#endif
    
    return 0;
}


static void dev_tmr_callback(void *arg)
{
    task_msg_post(TASK_DEV, EVT_TIMER, 0, NULL, 0);
}

////////////////////////////////////////////////




static U32 tmr_cnt=0;
static int timer_proc(void)
{
    int r;
    
    get_stat(&curStat);
    if(sysState==STAT_POWEROFF) {
        paras_set_state(STAT_STOP);
        if(curStat.aPres>0 && curStat.dPres>0 && (curStat.aPres-curStat.dPres<2.0F)) {
            valve_set(VALVE_CLOSE);
        }
    }
    else if(sysState!=STAT_UPGRADE) {
        paras_set_state(sysState);
        if(sysState==STAT_RUNNING && adjMode==ADJ_AUTO) {
            auto_pres(&curStat);
        }
    }
              
    return r;
}


void task_dev_fn(void *arg)
{
    int r;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    tmrId = osTimerNew(dev_tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
    
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            
            switch(e.evt) {
                case EVT_TIMER:
                {
                    r = timer_proc();
                }
                break;
                
                default:
                break;
            }
        }
        
    }
}



