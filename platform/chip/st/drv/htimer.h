#ifndef __HTIMER_Hx__
#define __HTIMER_Hx__

#include "types.h"

typedef void (*htimer_callback_t)(void *arg);


typedef struct {
    int                 ms;
    int                 freq;
    int                 repeat;
    htimer_callback_t   callback;
    void                *user_data;
}htimer_set_t;


int htimer_init(void);
int htimer_deinit(void);
int htimer_new(void);

int htimer_set(int htimer_id, htimer_set_t *set);
int htimer_free(int htimer_id);
int htimer_start(int htimer_id);
int htimer_stop(int htimer_id);


#endif
