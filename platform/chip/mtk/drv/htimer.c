#include "drv/htimer.h"
#include "bio/bio.h"
#include "log.h"

#define TIMER_PORT HAL_GPT_1


typedef struct {
    hal_gpt_port_t  port;
    htimer_set_t     set;
    U8              stop;
    U32             tmr_id;
}hhtimer_handle_t;

static hal_gpt_port_t timer_port;
static void htimer_hw_callback(void *user_data)
{
    htimer_handle_t *h=(htimer_handle_t*)user_data;
    
    if(h && h->set.callback && !h->stop) {
        h->set.callback(user_data);
    }
}



int timer_init(hal_gpt_port_t port)
{
    hal_gpt_status_t st;

    timer_port = port;
    st = hal_gpt_init(timer_port);
    if(st) {
        LOGE("timer_start %d, error\r\n", st);
        return -1;
    }
    
    return 0;
}


handle_t htimer_new(void)
{
    htimer_handle_t   *h;
    
    h=calloc(1, sizeof(htimer_handle_t));
    if(!h) {
        return NULL;
    }
    
    return h;
}



int htimer_free(handle_t *h)
{
    htimer_handle_t **hh=((htimer_handle_t**)h);
    
    if(!hh || !(*hh)) {
        return -1;
    }
    
    
    free(*hh);
    
    return 0;
}


int htimer_get(handle_t h, htimer_set_t *set)
{
    if(!h || !set) {
        return -1;
    }
    
    *set = ((htimer_handle_t*)h)->set;
    return 0;
}


int htimer_set(handle_t h, htimer_set_t *set)
{
    if(!h || !set) {
        return -1;
    }
    
    ((htimer_handle_t*)h)->set = *set;
    return 0;
}


int htimer_start(handle_t h)
{
    hal_gpt_status_t st;
    htimer_set_t      *set;
    
    if(!h) {
        return -1;
    }
    
    st = hal_gpt_register_callback(timer_port, htimer_hw_callback, h);
    if(st) {
        LOGE("hal_gpt_register_callback, error, %d\r\n", st);
        return -1;
    }
    
    set = &((htimer_handle_t*)h)->set;
    if(set->ms>0 && set->freq==0) {
        st = hal_gpt_start_timer_ms(timer_port, set->ms, (set->repeat>0)?HAL_GPT_TIMER_TYPE_REPEAT:HAL_GPT_TIMER_TYPE_ONE_SHOT);
    }
    else if(set->freq>0 && set->ms==0) {
        st = hal_gpt_start_timer_Hz(timer_port, set->freq, (set->repeat>0)?HAL_GPT_TIMER_TYPE_REPEAT:HAL_GPT_TIMER_TYPE_ONE_SHOT);
    }
    else {
        st = (hal_gpt_status_t)-1;
    }
    if(st) {
        LOGE("timer_start %d, error\r\n", st);
        return -1;
    }
    
    return 0;
}


int htimer_stop(handle_t h)
{
    hal_gpt_status_t st;
    
    if(!h) {
        return -1;
    }
    
    st = hal_gpt_stop_timer(timer_port);
    if(st) {
        return -1;
    }
    
    return 0;
}

//////////////////////////////////////////////////////
static void htimer_sw_callback(void *user_data)
{
    htimer_handle_t *h=(htimer_handle_t*)user_data;
    
    if(h && h->set.callback && h->stop==0) {
        h->set.callback(user_data);
    }
    
    if(h->set.repeat) {
        hal_gpt_sw_start_timer_ms(h->tmr_id, h->set.ms, htimer_sw_callback, h);
    }
}


handle_t htimer_sw_start(htimer_set_t *set)
{
    U32 tmp;
    hal_gpt_status_t st;
    htimer_handle_t *h=(htimer_handle_t*)malloc(sizeof(htimer_handle_t));
    
    if(!h || !set) {
        return NULL;
    }
    
    st = hal_gpt_sw_get_timer(&tmp);
    if(st) {
        free(h);
        return NULL;
    }
    
    st = hal_gpt_sw_start_timer_ms(tmp, set->ms, htimer_sw_callback, h);
    if(st) {
        free(h);
        hal_gpt_sw_free_timer(tmp);
        return NULL;
    }
    
    h->tmr_id = tmp;
    h->set = *set;
    h->stop = 0;
    
    return h;
}



int htimer_sw_stop(handle_t *h)
{
    if(!h) {
        return -1;
    }
    
    htimer_handle_t *th=*((htimer_handle_t**)h);
    hal_gpt_sw_stop_timer_ms(th->tmr_id);
    hal_gpt_sw_free_timer(th->tmr_id);
    free(th);
    
    return 0;
}















