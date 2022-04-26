#include "key.h"
#include "drv/clk.h"
#include "drv/delay.h"



typedef struct {
    U32             up;             //ms
    U32             down;           //ms
}key_tick_t;

typedef struct {
    U32             up;             //ms
    U32             down;           //ms
}key_time_t;

typedef struct {
    key_set_t       set;
    key_tick_t      tik;
    
    U16             times;
    handle_t        handle;
}key_data_t;

typedef struct {
    U8          cnt;
    key_data_t  *data[KEY_MAX];
}mode_data_t;

typedef struct {
    mode_data_t     isr;
    mode_data_t     poll;
    
    key_data_t      data[KEY_MAX];
}key_all_t;


static key_tab_t keyTab={0};
static key_all_t keyAll={0};

static void key_tab_reset(key_tab_t *tab)
{
    tab->cnt = 0;
}
static void key_tab_add(key_tab_t *tab, key_t *key)
{
    if(key->key!=KEY_NONE) {
        tab->tab[tab->cnt] = *key;
        tab->cnt++;
    }
}

static void key_all_reset(key_all_t *all)
{
    memset(all, 0, sizeof(keyAll));
}

static void key_callback(void)
{
    
    
}

static void key_poll(key_all_t *all, key_tab_t *tab)
{
    U8 i;
    U32 tik;
    key_t key;
    key_data_t *kdat;
    
    key_tab_reset(tab);
    
    tik = clk2_get_tick();
    for(i=0; i<all->poll.cnt; i++) {
        
        key.key = KEY_NONE;
        kdat = all->poll.data[i];
        if(gpio_get_hl(kdat->handle)==kdat->set.pressLevel) { //如果有键按下
            kdat->tik.up = 0;
            if(kdat->tik.down==0) {                           //第一次检测到按下
                kdat->tik.down = tik;
            }
            else {                                          //连续多次检测到按下
                if(kdat->set.longPressBurst) {
                    if((tik-kdat->tik.down)%kdat->set.longPressTime==0) {    //continue send key
                        if(kdat->times<kdat->set.longPressBurstTimes) {
                            key.key = kdat->set.key;
                            key.updown = PRESS_DOWN;
                            key.bLongPress = 1;
                            
                            kdat->times++;
                        }
                    }
                }
                else {
                    if((tik-kdat->tik.down)>=kdat->set.longPressTime && kdat->times==0) {
                        key.key = kdat->set.key;
                        key.updown = PRESS_DOWN;
                        key.bLongPress = 1;
                        
                        kdat->times++;
                    }
                }
            }
        }
        else {
            if(kdat->tik.down>0) {
                
                if(kdat->tik.up==0) {                             //第一次检测到按键松开
                
                    key.key = kdat->set.key;
                    key.updown = PRESS_UP;
                    key.bLongPress = 0;
                    
                    kdat->times = 0;
                    kdat->tik.down = 0;
                }
                
                kdat->tik.up = tik;
            }
        }
        
        key_tab_add(tab, &key);
    }
}

/////////////////////////////////////

int key_init(key_set_t *set)
{
    U8 i=0;
    mode_data_t *pd=NULL;
    
    gpio_cfg_t cfg;
    key_all_t *all=&keyAll;
    
    if(!set) {
        return -1;
    }
    
    key_all_reset(all);
    while(1) {
        
        if(set[i].key==KEY_NONE) {
            break;
        }
        
        pd = NULL;
        all->data[i].set = set[i];
        if(set[i].mode==MODE_ISR) {
            cfg.pin = set[i].pin;
            cfg.mode = MODE_EVT_BOTH;
            //cfg.irq.prep = 6;
            //cfg.irq.subp = 0;
            
            pd = &all->isr;
        }
        else if(set[i].mode==MODE_SCAN) {
            cfg.pin = set[i].pin;
            cfg.mode = MODE_INPUT;
            
            pd = &all->poll;
        }
        
        if(pd) {
            //all->data[i].handle = gpio_init(&cfg);
            
            pd->data[pd->cnt] = &all->data[i];
            pd->cnt++;
        }
        
        i++;
    }
    
    return 0;
}



int key_set_callback(key_callback_t kcb)
{
    if(!kcb) {
        return -1;
    }
    
    return 0;
}


int key_get(key_tab_t *tab)
{
    if(!tab) {
        return -1;
    }
    
    key_poll(&keyAll, &keyTab);
    memcpy(tab, &keyTab, sizeof(key_tab_t));
    
    return 0;
}




void key_test(void)
{
    int i;
    key_tab_t tab;
    key_t key;
    U32   startTime=0;
    enum { STAT_START, STAT_STOP};
    int  status=STAT_STOP;
    #define SW_TIME  30000
    
    
    key_set_t set[]={
        {KEY_UP,    {GPIOA, GPIO_PIN_0}, MODE_SCAN, 0, 200, 0, 0, NULL},
        {KEY_DOWN,  {GPIOA, GPIO_PIN_0}, MODE_SCAN, 0, 200, 0, 0, NULL},
        {KEY_ENTER, {GPIOA, GPIO_PIN_0}, MODE_SCAN, 0, 200, 0, 0, NULL},
    
        {KEY_NONE,  {NULL,  0},          0,         0, 0,   0, 0, NULL},
    };
    
    
    
    key_init(set);
    while(1) {
        
        key_get(&tab);
        if(tab.cnt>0) {
            delay_ms(10);
            key_get(&tab);
        }
        
        
        for(i=0; i<tab.cnt; i++) {
            key = tab.tab[i];
            
            switch(key.key) {
                
                case KEY_UP:
                break;
                
                case KEY_DOWN:
                break;
                
                case KEY_ENTER:
                {
                    if(status==STAT_STOP) {
                        status = STAT_START;
                        startTime = clk2_get_tick();
                    }
                    else {
                        status = STAT_STOP;
                        startTime = 0;
                    }
                }
                break;
            }
        }
        
        
        if(startTime>0 && (clk2_get_tick()-startTime>=SW_TIME)) {
            //switch lead channel
            
        }
    }
    
    
}














