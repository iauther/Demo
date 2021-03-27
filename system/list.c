#include <stdlib.h>
#include <string.h>
#include "list.h"



list_t* list_init(int max, int node_size)
{
    u8 *ps;
    int i,hlen,nlen;
    list_t *l=NULL;
    lnode_t *pn;

    if(max<=0 || node_size<=0) {
        return NULL;
    }

    l = (list_t*)malloc(sizeof(list_t));
    if(!l) {
        return NULL;
    }
    
    hlen = max*sizeof(lnode_t);
    nlen = max*node_size;
    l->pool = (lnode_t*)malloc(hlen+nlen);
    if(!l->pool) {
        free(l);
        return NULL;
    }
    
    ps = (u8*)l->pool+hlen;
    for(i=0; i<max; i++) {
        pn = l->pool+i;
        pn->data.ptr = ps+node_size*i;
        pn->data.len = node_size;
        pn->prev = (i==0)?NULL:(pn-1);
        pn->next = (i==max-1)?NULL:(pn+1);
    }

    l->max  = max;
    l->size = 0;
    l->node_size = node_size;

    return l;
}


int list_append(list_t *l, node_t *n)
{
    lnode_t *pn;

    if(!l || l->size>=l->max) {
        return -1;
    }

    pn = l->pool+l->size;
    if(n && n->ptr) {
        memcpy(pn->data.ptr, n->ptr, MIN(l->node_size, n->len));
    }
    l->size++;

    return 0;
}


int list_get(list_t *l, int index, node_t *n)
{
    lnode_t *pn;

    if(!l || !l->size || index>=l->size) {
        return -1;
    }

    pn = l->pool+index;
    if(n && n->ptr) {
        memcpy(n->ptr, pn->data.ptr, MIN(l->node_size, n->len));
    }

    return 0;
}


int list_set(list_t *l, int index, node_t *n)
{
    lnode_t *pn;

    if(!l || index>=l->size) {
        return -1;
    }

    pn = l->pool+l->size;
    if(n && n->ptr) {
        memcpy(pn->data.ptr, n->ptr, MIN(l->node_size, n->len));
    }

    return 0;
}


int list_size(list_t *l)
{
    return l->size;
}


int list_quit(list_t *l)
{
    l->quit = 1;
    return 0;
}


int list_clear(list_t *l)
{
    return 0;
}


int list_free(list_t **l)
{
    return 0;
}


