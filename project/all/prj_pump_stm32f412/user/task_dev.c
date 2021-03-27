#include "task.h"                          // CMSIS RTOS header file
#include "data.h" 
#include "n950.h"
#include "valve.h"
#include "ms4525.h"
#include "bmp280/bmp280.h"
#include "drv/delay.h"


#define ACC                 1.0F        //accuracy, 1kpa

#define VMIN(v)             (v-ACC)
#define VMAX(v)             (v+ACC)
#define TIMER_MS            50         //ms


enum {
    PRES_RAISE=0,
    PRES_LEAK,
    PRES_KEEP,
};

extern setts_t DEFAULT_SETTS;

static U8 presMode=PRES_RAISE;
static U8 vacuum_ever_reached=0;
static stat_t curStat;
static U16 prevSpeed=0;
extern int com_send(U8 type, U8 nAck, void *data, U16 len);


static void load_setting(void)
{
    //read_flash and load settings
    curSetts = DEFAULT_SETTS;
}

static void auto_pres(stat_t *st)
{
    U16 speed=0;
    sett_t *set=&curSetts.sett[curSetts.mode];
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
        n950_send_cmd(CMD_SET_SPEED, speed);
        prevSpeed = speed;
    }
        
}

static int get_stat(stat_t *st)
{
    ms4525_t ms;
    bmp280_t bmp;
    
    ms4525_get(&ms);
    bmp280_get(&bmp);
    n950_get(&st->pump);
    
    st->dPres = ms.pres;
    st->aPres = bmp.pres;
    st->temp  = bmp.temp;
    
    com_send(TYPE_STAT, 0, st, sizeof(stat_t));
    
    return 0;
}


static void tmr_callback(void *arg)
{
    task_msg_post(TASK_DEV, EVT_TIMER, 0, NULL, 0);
}

////////////////////////////////////////////////





static int timer_proc(void)
{
    int r;
    
    get_stat(&curStat);
    auto_pres(&curStat);
              
    return r;
}





static void dev_init(void)
{
    load_setting();
    
    n950_init();
    ms4525_init();
    bmp280_init();
    valve_init();
}


void task_dev_fn(void *arg)
{
    int r;
    evt_t e;
    osTimerId_t tmrId;
    task_handle_t *h=(task_handle_t*)arg;
    
    dev_init();
    tmrId = osTimerNew(tmr_callback, osTimerPeriodic, NULL, NULL);
    osTimerStart(tmrId, TIMER_MS);
    h->running = 1;
    
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



