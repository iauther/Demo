#include "list.h"
#include "lock.h"


#ifndef LIST_MALLOC
#define LIST_MALLOC malloc
#endif

#ifndef LIST_FREE
#define LIST_FREE free
#endif

#ifndef LIST_COPY
#define LIST_COPY memcpy
#endif


typedef struct list_node {
    struct list_node    *prev;
    struct list_node    *next;
    node_t              data;
}list_node_t;

typedef struct {
    list_node_t *head;
    list_node_t *tail;
    U32         size;
  
    handle_t    lock;
    list_cfg_t  cfg;
} list_t;


static list_node_t *node_new(void *data, int len)
{
    list_node_t *node=LIST_MALLOC(sizeof(list_node_t));
    
    if (!node) return NULL;
        
    node->prev = NULL;
    node->next = NULL;
    node->data.ptr = LIST_MALLOC(len);
    if(node->data.ptr) {
        LIST_COPY(node->data.ptr, data, len);
        node->data.len = len;
    }
    return node;
}
static int node_free(list_node_t *node)
{
    if(!node) {
        return -1;
    }
    
    LIST_FREE(node->data.ptr);
    LIST_FREE(node);
    
    return 0;
}
static int node_set(list_node_t *node, node_t *nd)
{
    if(!node) {
        return -1;
    }
    
    LIST_FREE(node->data.ptr);
    node->data.ptr = LIST_MALLOC(nd->len);
    if(node->data.ptr) {
        LIST_COPY(node->data.ptr, nd->ptr, nd->len);
        node->data.len = nd->len;
    }
    
    return 0;
}



static list_node_t *node_at(list_t *l, int index)
{
    int idx=0;
    list_node_t *tmp=l->head;
    
    while(tmp) {
        if(idx==index) {
            return tmp;
        }
        tmp = tmp->next;
        idx++;
    }
    
    return NULL;
}
///////////////////////////////////////////////////////////
handle_t list_new(list_cfg_t *cfg)
{
    list_t *l=LIST_MALLOC(sizeof(list_t));
    if (!cfg || !l)
        return NULL;
    
    l->head = NULL;
    l->tail = NULL;
    l->size = 0;
    l->lock = lock_dynamic_new();
    l->cfg = *cfg;
    
    return l;
}


int list_destroy(handle_t l)
{
    list_node_t *next;
    list_t *hl=(list_t*)l;
    
    if(!hl) {
        return -1;
    }
    
    U32 size = hl->size;
    list_node_t *cur = hl->head;
    
    lock_dynamic_hold(hl->lock);
    while (size--) {
        next = cur->next;
        node_free(cur);
        cur = next;
    }
    LIST_FREE(hl);
    lock_dynamic_release(hl->lock);
    
    return 0;
}


int list_get(handle_t l, node_t *node, U32 index)
{
    list_node_t *nd;
    list_t *hl=(list_t*)l;
    
    if(!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    nd = node_at(hl, index);
    lock_dynamic_release(hl->lock);
    
    if(!nd) {
        return -1;
    }
    if(node) *node = nd->data;
    
    return 0;
}


int list_set(handle_t l, node_t *node, U32 index)
{
    list_node_t *nd;
    list_t *hl=(list_t*)l;
    
    if(!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    nd = node_at(hl, index);
    if(nd) {
        node_set(nd, node);
    }
    lock_dynamic_release(hl->lock);
    
    return nd?0:-1;
}



int list_add(handle_t l, node_t *node, U32 index)
{
    int idx=0;
    list_t *hl=(list_t*)l;
    list_node_t *nd,*tmp;
    
    if (!hl || !node || index>hl->size) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    
    nd = node_at(l, index);
    tmp = node_new(node->ptr, node->len);
    if(tmp) {
        
        if(index==0) {        //head
            tmp->prev = NULL;
            tmp->next = nd;
            
            nd->prev = tmp;
            hl->head = tmp;
        }
        else if(index==(hl->size-1)) {   //tail
            
            tmp->prev = nd;
            tmp->next = NULL;
            
            nd->next = tmp;
            hl->tail = tmp;
        }
        else {
            nd->prev->next = tmp;
            nd->prev = tmp;
            
            tmp->prev = nd->prev;
            tmp->next = nd;
        }
        
        hl->size++;
    }
    lock_dynamic_release(hl->lock);
    
    return 0;
}


int list_append(handle_t l, void *data, U32 len)
{
    list_t *hl=(list_t*)l;
    node_t node={data, len};
    
    if(!hl) {
        return -1;
    }
    
    if(hl->cfg.max==hl->size) {
        if(hl->cfg.mode==MODE_FIFO) {
            list_remove(l, 0);
        }
        else {
            return -1;
        }
    }
    
    return list_add(l, &node, hl->size-1);
}




int list_remove(handle_t l, U32 index)
{
    list_node_t *node;
    list_t *hl=(list_t*)l;
    
    if (!hl || hl->size==0 || index>=hl->size) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    node = node_at(hl, index);
    
    node->prev
    ? (node->prev->next = node->next)
    : (hl->head = node->next);

    node->next
    ? (node->next->prev = node->prev)
    : (hl->tail = node->prev);

    node_free(node);
    hl->size--;
    lock_dynamic_release(hl->lock);
    
    return 0;
}


int list_iterator(handle_t l, node_t *node, list_callback_t callback)
{
    int r;
    list_node_t *nd;
    list_t *hl=(list_t*)l;

    if (!hl || !node || hl->size==0) {
        return -1;
    }

    lock_dynamic_hold(hl->lock);
    
    nd = hl->head;
    while (nd) {
        if (callback) {
            r = callback(hl, node, nd);
            if(r<0) {
                break;
            }
        }
        
        nd = nd->next;
    }
    lock_dynamic_hold(hl->lock);
    
    return NULL;
}

