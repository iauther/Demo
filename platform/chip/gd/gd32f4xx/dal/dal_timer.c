#include "dal_timer.h"
#include "gd32f4xx_timer.h"


#define TMR_FREQ_MAX        (60*1000000)

typedef struct {
    U32                 timer;
    rcu_periph_enum     rcu;
    
    U8                  IRQn;
    
}timer_info_t;


const timer_info_t timerInfo[TIMER_MAX]={

    {TIMER1, RCU_TIMER1, TIMER1_IRQn},
    {TIMER2, RCU_TIMER2, TIMER2_IRQn},
    {TIMER3, RCU_TIMER3, TIMER3_IRQn},
    {TIMER4, RCU_TIMER4, TIMER4_IRQn},
    {TIMER5, RCU_TIMER5, TIMER5_DAC_IRQn},
    {TIMER6, RCU_TIMER6, TIMER6_IRQn},
};


typedef struct {
    dal_timer_cfg_t    cfg;
    const timer_info_t *info;
    int                times;
}timer_handle_t;

static timer_handle_t* timerHandle[TIMER_MAX]={NULL};

static void tmr_init(void)
{
    //定时器使能
    rcu_periph_clock_enable(RCU_TIMER6);
    timer_prescaler_config(TIMER6, 9, TIMER_PSC_RELOAD_UPDATE);
    timer_autoreload_value_config(TIMER6, 0xff);
    timer_master_output_trigger_source_select(TIMER6, TIMER_TRI_OUT_SRC_UPDATE);
    timer_enable(TIMER6);
}


//timer1~timer6 APB1, freq max:60MHz
handle_t dal_timer_init(dal_timer_cfg_t *cfg)
{
    timer_parameter_struct init;
    timer_handle_t *h=malloc(sizeof(dal_timer_cfg_t));
    
    if(!cfg || cfg->timer>=TIMER_MAX) {
        return NULL;
    }
    
    h = malloc(sizeof(dal_timer_cfg_t));
    if(!h) {
        return NULL;
    }
    h->cfg = *cfg;
    h->info = &timerInfo[h->cfg.timer];
    h->times = 0;
    timerHandle[h->cfg.timer] = h;
    
    rcu_apb1_clock_config(RCU_APB1_CKAHB_DIV1);
    //rcu_apb2_clock_config(RCU_APB2_CKAHB_DIV1);
    
    rcu_periph_clock_enable(h->info->rcu);
    //timer_internal_clock_config(h->info->timer);

    timer_deinit(h->info->timer);
    timer_struct_para_init(&init);
    
    init.prescaler        = 1;
    init.alignedmode      = TIMER_COUNTER_EDGE;
    init.counterdirection = TIMER_COUNTER_UP;
    init.period           = TMR_FREQ_MAX/h->cfg.freq;
    init.clockdivision    = TIMER_CKDIV_DIV1;
    init.repetitioncounter = 0;
    timer_init(h->info->timer, &init);

    timer_auto_reload_shadow_enable(h->info->timer);
    timer_interrupt_flag_clear(h->info->timer, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(h->info->timer, TIMER_INT_UP);

    //timer_enable(h->info->timer);
    nvic_irq_enable(h->info->IRQn, 2, 0);
    
    return h;
}



int dal_timer_deinit(handle_t h)
{
    timer_handle_t *th=(timer_handle_t*)h;
    
    if(!th) {
        return -1;
    }
    
    timer_deinit(th->info->timer);
    free(th);
    
    return 0;
}


int dal_timer_en(handle_t h, U8 on)
{
    timer_handle_t *th=(timer_handle_t*)h;
    
    if(!th) {
        return -1;
    }
    
    if(on) {
        timer_enable(th->info->timer);
    }
    else {
        timer_disable(th->info->timer);
    }
    
    return 0;
}



static void timerX_handler(U8 timer)
{
    timer_handle_t *h=timerHandle[timer];
    
    if(timer_interrupt_flag_get(h->info->timer, TIMER_INT_FLAG_UP)) {
        timer_interrupt_flag_clear(h->info->timer, TIMER_INT_FLAG_UP);
        
		if(((h->cfg.times<0) || ((h->cfg.times>0) && (h->times<h->cfg.times-1))) && h->cfg.callback) {
            h->cfg.callback(h, h->cfg.arg);
            h->times++;
        }
    }
}


void TIMER1_IRQHandler(void){timerX_handler(TIMER_1);}
void TIMER2_IRQHandler(void){timerX_handler(TIMER_2);}
void TIMER3_IRQHandler(void){timerX_handler(TIMER_3);}
void TIMER4_IRQHandler(void){timerX_handler(TIMER_4);}
void TIMER5_IRQHandler(void){timerX_handler(TIMER_5);}
void TIMER6_IRQHandler(void){timerX_handler(TIMER_6);}












