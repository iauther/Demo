#ifndef __KEY_H__
#define __KEY_H__

#include "types.h"
#include "drv/gpio.h"


enum {
    MODE_INT=0,
    MODE_SCAN,
};

enum {
    PRESS_LONG=0,
    PRESS_SHORT,
};

enum {
    PRESS_DOWN=0,
    PRESS_UP,
};

enum {
    KEY_UP=0,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_EXIT,
    KEY_BACK,
    KEY_ENTER,
};

typedef struct {
    U8          key;
    gpio_pin_t  pin;
}key_map_t;

typedef struct {
    U8 key;
    U8 updown;
    U8 bLongPress;
}key_t;

typedef int (*key_cb_fn)(key_t *key);


typedef struct {
    U8  mode;               //int or scan
    key_cb_fn cb;
    U16 longPressTime;       //ms
    U8 longPressBurst;
    key_map_t *map;
}key_cfg_t;


int key_init(key_cfg_t *cfg);

int key_set_callback(key_cb_fn cb);

#endif

