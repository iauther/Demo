#ifndef __LOG_Hx__
#define __LOG_Hx__

#include "types.h"

typedef enum {
    LV_INFO,
    LV_DEBUG,
    LV_WARNNING,
    LV_ERROR,
    
    LV_MAX
}LEVEL;


typedef struct {
    U8      en[LV_MAX];
}log_cfg_t;

extern log_cfg_t log_cfg;
extern const char* log_string[LV_MAX];

int log_get(log_cfg_t *cfg);

int log_set(log_cfg_t *cfg);

#if 0
#define LOG(lv, fmt, ...)   if(log_cfg.en[lv]) {\
                                printf("%s %s, line:%d, ", log_string[lv], __FILE__, __LINE__);\
                                printf(fmt, ##__VA_ARGS__);\
                            }
#else
#define LOG(lv, fmt, ...)   if(log_cfg.en[lv]) {\
                                printf(fmt, ##__VA_ARGS__);\
                            }
#endif


#define LOGI(fmt, ...)      LOG(LV_INFO, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)      LOG(LV_DEBUG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)      LOG(LV_WARNNING, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...)      LOG(LV_ERROR, fmt, ##__VA_ARGS__)
                            
#endif
