#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "types.h"

typedef struct {
	node_t     *nodes;		// array to store queue elements
	int         max;	    // maximum capacity of the queue
	int         size;		// element counter in the queue
	int         head;		// head points to head element in the queue (if any)
	int         tail;		// tail points to last element in the queue

    int         quit;
    int         locked;
    int         bsz;
    
}queue_t;


typedef int (*qiterater)(queue_t *q, int index, void *p1, void *p2);


queue_t* queue_init(int max, int bsz);

int queue_free(queue_t **q);

int queue_size(queue_t *q);

int queue_capacity(queue_t *q);

int queue_get(queue_t *q, node_t *n, qiterater iter);

int queue_put(queue_t *q, node_t *n, qiterater iter);

int queue_pop(queue_t *q);

int queue_peer(queue_t *q, node_t *n);

int queue_iterate(queue_t *q, node_t *n, qiterater iter);

int queue_clear(queue_t *q);

#endif
