#ifndef __KEY_H__
#define __KEY_H__

#include "types.h"
#include "dal_gpio.h"


enum {
    MODE_ISR=0,
    MODE_SCAN,
};

enum {
    PRESS_DOWN=0,
    PRESS_UP,
};

enum {
    KEY_NONE=0,
    
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_EXIT,
    KEY_BACK,
    KEY_ENTER,
    
    KEY_MAX
};

typedef struct {
    U8          key;
    U8          updown;
    U8          bLongPress;
}key_t;

typedef struct {
    U8              cnt;
    key_t           tab[KEY_MAX];
}key_tab_t;

typedef int (*key_callback_t)(key_t *key);

typedef struct {
    U8          key;
    dal_pin_t   pin;
    
    U8          mode;               //int or scan
    U8          pressLevel;
    U16         longPressTime;      //how long time is long press
    U8          longPressBurst;
    U16         longPressBurstTimes;
    
    key_callback_t callback;
}key_set_t;


int key_init(key_set_t *set);

int key_set_callback(key_callback_t kcb);

int key_get(key_tab_t *tab);

void key_test(void);

#endif

