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

typedef struct {
    U8  mode;
    U32 max;        //the max count
}list_cfg_t;



//该函数返回负值是表示出错，将退出迭代循环
//该函数返回值>=0将继续迭代
typedef int (*list_callback_t)(handle_t l, node_t *nd, void *ndx);


handle_t list_new(list_cfg_t *cfg);

int list_get(handle_t l, node_t *node, U32 index);
int list_set(handle_t l, node_t *node, U32 index);
int list_add(handle_t l, node_t *node, U32 index);
int list_append(handle_t l, void *data, U32 len);
int list_remove(handle_t l, U32 index);
int list_destroy(handle_t l);
int list_iterator(handle_t l, node_t *node, list_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif


