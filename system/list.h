#ifndef __LIST_Hx__
#define __LIST_Hx__

#include "types.h"

typedef struct _list {
    int             max;
    int             size;
    int             node_size;
    lnode_t         *pool;
    U8              quit;
}list_t;

list_t* list_init(int max, int node_size);

int list_append(list_t *l, node_t *n);

int list_get(list_t *l, int index, node_t *n);

int list_set(list_t *l, int index, node_t *n);

int list_size(list_t *l);

int list_quit(list_t *l);

int list_clear(list_t *l);

int list_free(list_t **l);

#endif
