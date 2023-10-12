#ifndef __MYLOG_Hx__
#define __MYLOG_Hx__

#include "wtl.h"

typedef enum {
    LV_INFO,
    LV_DEBUG,
    LV_WARN,
    LV_ERROR,

    LV_MAX
}LOG_LEVEL;

typedef struct {
    int  enable;
    int  color;
}level_t;

void log_init(HWND hwnd, CRect rc, CFont font);
HWND log_get_hwnd(void);
void log_enable(int flag);
int log_is_enable(void);
void log_clear(void);
int log_save(const char* path);
void log_set_level(LOG_LEVEL lv, level_t* ld);
void log_print(LOG_LEVEL lv, const char* format, ...);

#define LOGx(lv, fmt, ...)   log_print(lv, fmt, ##__VA_ARGS__)

#define LOG                 LOGx
#define LOGI(fmt, ...)      LOGx(LV_INFO,  fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)      LOGx(LV_DEBUG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)      LOGx(LV_WARN,  fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...)      LOGx(LV_ERROR, fmt, ##__VA_ARGS__)

#endif


































