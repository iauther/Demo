#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

static int do_iterate(queue_t *q, void *p, qiterater iter)
{
    int i,o,r=-1;
    node_t *n;

    for(i=0; i<q->size; i++) {
        o = (q->head+i)%q->max;
        n = &q->nodes[o];
        r = iter(q, i, p, n->ptr);
        if(q->quit || r>=0) {
            break;
        }
    }

    return r;
}
queue_t* queue_init(int max, int bsz)
{
    int i;
    node_t n;
    u8 *p,*ptr;

	queue_t *q = (queue_t*)malloc(sizeof(queue_t));
	if(!q) {
        return NULL;
    }
    
    p = (u8*)malloc(max*(sizeof(node_t)+bsz));
    if(!p) {
        return NULL;
    }

    ptr = p+max*sizeof(node_t);
    q->nodes = (node_t*)p;
    for(i=0; i<max; i++) {
        q->nodes[i].ptr = ptr+i*bsz;
        q->nodes[i].len = bsz;
    }
	q->max  = max;
	q->size = 0;
	q->head = 0;
	q->tail = 0;

    q->quit = 0;
    q->locked = 0;
    q->bsz = bsz;

	return q;
}


int queue_free(queue_t **q)
{
    if(!q) {
        return -1;
    }

    free(*q);
    free((*q)->nodes);
    
    return 0;
}


int queue_size(queue_t *q)
{
	return q?q->size:-1;
}


int queue_capacity(queue_t *q)
{
	return q?q->max:-1;
}


int queue_put(queue_t *q, node_t *n, qiterater iter)
{
    int i=-1;
    node_t *n2;

	if (!q || !n || q->size==q->max || q->locked) {
		return -1;      //queue full
	}
    
    q->locked = 1;
    if(iter && q->size>0) {
        i = do_iterate(q, n->ptr, iter);
    }

    if(i<0) {       //not find
        i = q->tail;
        q->tail = (q->tail+1)%q->max;
        q->size++;
    }
    n2 = &q->nodes[i];
    n2->len = q->bsz;
    if(n2->ptr) {
        memcpy(n2->ptr, n->ptr, n2->len);
    }
    q->locked = 0;
	
	return 0;
}

 
int queue_get(queue_t *q, node_t *n, qiterater iter)
{
    int i=-1;

	if (!q || !n || q->size==0 || q->locked) {
		return -1;
	}
    
    q->locked = 1;
    if(iter && q->size>0) {
        i = do_iterate(q, n->ptr, iter);
        if(i<0) {
            q->locked = 0;
            return -1;
        }
    }
    
    if(i<0) {
        i = q->head;
        q->head = (q->head + 1) % q->max;	// circular queue
    }
    
    if(n->ptr) {
        memcpy(n->ptr, q->nodes[i].ptr, q->nodes[i].len);
    }
    n->len = q->nodes[i].len;
	
	q->size--;
    q->locked = 0;
	
	return 0;
}


int queue_pop(queue_t *q)
{
    int i=-1;

	if (!q || q->size==0 || q->locked) {
		return -1;
	}
    
    q->locked = 1;
    if(i<0) {
        i = q->head;
        q->head = (q->head + 1) % q->max;	// circular queue
    }
	
	q->size--;
    q->locked = 0;
	
	return 0;
}


int queue_peer(queue_t *q, node_t *n)
{
    int i=-1;

	if (!q || !n || q->size==0 || q->locked) {
		return -1;
	}
    
    q->locked = 1;
    i = q->head;
    memcpy(n->ptr, q->nodes[i].ptr, q->nodes[i].len);
    n->len = q->nodes[i].len;
    q->locked = 0;
	
	return 0;
}


int queue_iterate(queue_t *q, node_t *n, qiterater iter)
{
    int r;

    if (!q || !iter || q->size==0 || q->locked) {
		return -1;
	}

    q->locked = 1;
    if(iter) {
        r = do_iterate(q, n, iter);
    }
    q->locked = 0;

    return r;
}


int queue_clear(queue_t *q)
{
    if (!q) {
		return -1;
	}
    
    q->size = 0;
	q->head = 0;
	q->tail = -1;
    q->locked = 0;
    q->quit = 0;

    return 0;
}

