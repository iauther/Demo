#ifndef __LOG_Hx__
#define __LOG_Hx__

#ifdef _WIN32
#include "myLog.h"
#else
#include "types.h"

typedef enum {
    LV_INFO,
    LV_DEBUG,
    LV_WARN,
    LV_ERROR,

    LV_MAX
}LOG_LEVEL;

typedef struct {
    int     en;
    int     max;
}lv_cfg_t;

typedef struct {
    lv_cfg_t   lv[LV_MAX];
}log_cfg_t;

extern log_cfg_t log_cfg;
extern const char* log_string[LV_MAX];

int log_init(rx_cb_t callback);
int log_deinit(void);
int log_enable(int en);
int log_set(LOG_LEVEL lv, int en);
int log_set_callback(rx_cb_t cb);
int log_print(LOG_LEVEL lv, char *fmt, ...);
int log_save(LOG_LEVEL lv);
int log_save_all(void);
int log_set_handle(handle_t h);
handle_t log_get_handle(void);


#define LOGx(lv, fmt, ...)  log_print(lv, fmt, ##__VA_ARGS__)

#define LOGI(fmt, ...)      LOGx(LV_INFO,  fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)      LOGx(LV_DEBUG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)      LOGx(LV_WARN,  fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...)      LOGx(LV_ERROR, fmt, ##__VA_ARGS__)

#endif
                                
#endif

