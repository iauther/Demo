#include "queue.h"

typedef struct {
	node_t     *nodes;		// array to store queue elements
	int         max;	    // maximum capacity of the queue
	int         size;		// element counter in the queue
	int         head;		// head points to head element in the queue (if any)
	int         tail;		// tail points to last element in the queue

    int         quit;
    int         locked;
    int         bsz;
}queue_handle_t;



static int do_iterate(handle_t h, void *p, qiterater iter)
{
    int i,o,r=-1;
    node_t *n;
    queue_handle_t *q=(queue_handle_t*)h;

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


handle_t queue_init(int max, int bsz)
{
    int i;
    node_t n;
    U8 *p,*ptr;
    queue_handle_t *h;

	h = (queue_handle_t*)malloc(sizeof(queue_handle_t));
	if(!h) {
        return NULL;
    }
    
    p = (U8*)malloc(max*(sizeof(node_t)+bsz));
    if(!p) {
        free(h);
        return NULL;
    }

    ptr = p+max*sizeof(node_t);
    h->nodes = (node_t*)p;
    for(i=0; i<max; i++) {
        h->nodes[i].ptr = ptr+i*bsz;
        h->nodes[i].len = bsz;
    }
	h->max  = max;
	h->size = 0;
	h->head = 0;
	h->tail = 0;
    
    h->quit = 0;
    h->locked = 0;
    h->bsz = bsz;

	return h;
}


int queue_free(handle_t *h)
{
    queue_handle_t **q=(queue_handle_t**)h;
    
    if(!q || !(*q)) {
        return -1;
    }

    free((*q)->nodes);
    free(*q);
    
    return 0;
}


int queue_size(handle_t h)
{
    queue_handle_t *q=(queue_handle_t*)h;
    
    if(!q ) {
        return -1;
    }
    
	return q?q->size:-1;
}


int queue_capacity(handle_t h)
{
    queue_handle_t *q=(queue_handle_t*)h;
    
    if(!q ) {
        return -1;
    }
    
	return q?q->max:-1;
}


int queue_put(handle_t h, node_t *n, qiterater iter)
{
    int i=-1;
    node_t *n2;
    queue_handle_t *q=(queue_handle_t*)h;
    

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

 
int queue_get(handle_t h, node_t *n, qiterater iter)
{
    int i=-1;
    queue_handle_t *q=(queue_handle_t*)h;

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


int queue_pop(handle_t h)
{
    int i=-1;
    queue_handle_t *q=(queue_handle_t*)h;

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


int queue_peer(handle_t h, node_t *n)
{
    int i=-1;
    queue_handle_t *q=(queue_handle_t*)h;

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


int queue_iterate(handle_t h, node_t *n, qiterater iter)
{
    int r;
    queue_handle_t *q=(queue_handle_t*)h;

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


int queue_clear(handle_t h)
{
    queue_handle_t *q=(queue_handle_t*)h;
    
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

