#ifndef __LIST_Hx__
#define __LIST_Hx__

#include <stdlib.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif


enum {
    MODE_FIFO=0,
    MODE_FILO,
};

typedef enum {
    ACT_NONE,
    ACT_STOP,
    ACT_REMOVE,
    ACT_DELETE,
    
}LIST_ACT;

typedef struct {
    U8          mode;
    int         max;        //the max count
}list_cfg_t;



//该函数返回负值是表示结束，将退出迭代循环
//该函数返回值>=0将继续迭代至结束
typedef int (*list_callback_t)(handle_t l, node_t *node, node_t *xd, void *arg, int *act);


handle_t list_init(list_cfg_t *cfg);
int list_free(handle_t l);
int list_get(handle_t l, node_t *node, int index);
int list_set(handle_t l, node_t *node, int index);

int list_take(handle_t l, node_t *node, int index);
int list_back(handle_t l, node_t *node);

int list_addto(handle_t l, node_t *node, int index);
int list_append(handle_t l, void *data, U32 len);
int list_remove(handle_t l, int index);
int list_delete(handle_t l, int index);
int list_size(handle_t l);
int list_clear(handle_t l);
int list_iterator(handle_t l, node_t *node, list_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif


