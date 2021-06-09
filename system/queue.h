#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "types.h"


typedef int (*qiterater)(handle_t h, int index, void *p1, void *p2);


handle_t queue_init(int max, int bsz);

int queue_free(handle_t *h);

int queue_size(handle_t h);

int queue_capacity(handle_t h);

int queue_get(handle_t h, node_t *n, qiterater iter);

int queue_put(handle_t h, node_t *n, qiterater iter);

int queue_iterate_quit(handle_t h);

int queue_pop(handle_t h);

int queue_peer(handle_t h, node_t *n);

int queue_iterate(handle_t h, node_t *n, qiterater iter);

int queue_clear(handle_t h);

#endif
