#include "list.h"
#include "lock.h"
#include "xmem.h"


typedef struct list_node {
    struct list_node    *prev;
    struct list_node    *next;
    node_t              data;
}list_node_t;

typedef struct {
    list_node_t *head;
    list_node_t *tail;
    int         size;
  
    handle_t    lock;
    list_cfg_t  cfg;
} list_t;


static list_node_t *node_new(list_t *l, void *data, int len)
{
    list_node_t *node=NULL;
    
    if(l->cfg.hmem) {
        node = xMalloc(l->cfg.hmem, sizeof(list_node_t), 0);
        if(node) {
            node->data.ptr = xMalloc(l->cfg.hmem, len, 0);
        }
    }
    else {
        node = malloc(sizeof(list_node_t));
        if(node) {
            node->data.ptr = malloc(len);
        }
    }
    
    if (!node) return NULL;
        
    node->prev = NULL;
    node->next = NULL;
    if(node->data.ptr) {
        memcpy(node->data.ptr, data, len);
        node->data.len = len;
    }
    return node;
}
static int node_free(list_t *l, list_node_t *node)
{
    if(!node) {
        return -1;
    }
    
    if(l->cfg.hmem) {
        xFree(l->cfg.hmem, node->data.ptr);
    }
    else {
        free(node->data.ptr);
        free(node);
    }
    
    return 0;
}
static int node_remove(list_t *l, list_node_t *node)
{
    return 0;
}


static int node_set(list_t *l, list_node_t *node, node_t *nd)
{
    if(!node) {
        return -1;
    }
    
    if(l->cfg.hmem) {
        xFree(l->cfg.hmem, node->data.ptr);
        node->data.ptr = xMalloc(l->cfg.hmem, nd->len, 0);
    }
    else {
        free(node->data.ptr);
        node->data.ptr = malloc(nd->len);
    }
    
    if(node->data.ptr) {
        memcpy(node->data.ptr, nd->ptr, nd->len);
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
    list_t *l=malloc(sizeof(list_t));
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
    U32 size;
    list_node_t *cur;
    list_node_t *next;
    list_t *hl=(list_t*)l;
    
    if(!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    size = hl->size; cur = hl->head;
    while (size--) {
        next = cur->next;
        node_free(hl, cur);
        cur = next;
    }
    lock_dynamic_release(hl->lock);
    lock_dynamic_free(hl->lock);
    free(hl);
    
    return 0;
}


int list_get(handle_t l, node_t *node, int index)
{
    list_node_t *nd;
    list_t *hl=(list_t*)l;
    
    if(!hl || !node) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    if (index>=hl->size) {
        lock_dynamic_release(hl->lock);
        return -1;
    }
    
    nd = node_at(hl, index);
    *node = nd->data;
    lock_dynamic_release(hl->lock);    
    
    return 0;
}


int list_set(handle_t l, node_t *node, int index)
{
    list_node_t *nd;
    list_t *hl=(list_t*)l;
    
    if(!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    if (index>=hl->size) {
        lock_dynamic_release(hl->lock);
        return -1;
    }
    
    nd = node_at(hl, index);
    if(nd) {
        node_set(hl, nd, node);
    }
    lock_dynamic_release(hl->lock);
    
    return nd?0:-1;
}



int list_add(handle_t l, node_t *node, int index)
{
    int idx=0;
    list_t *hl=(list_t*)l;
    list_node_t *xd,*tmp;
    
    if (!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    if (index>hl->size) {
        lock_dynamic_release(hl->lock);
        return -1;
    }
    
    xd = node_at(l, index);
    tmp = node_new(hl, node->ptr, node->len);
    if(tmp) {
        
        if(index==0) {                  //head
            tmp->prev = NULL;
            tmp->next = xd;
            
            xd->prev = tmp;
            hl->head = tmp;
        }
        else if((index==(hl->size-1)) || (index==-1)) {   //tail
            
            tmp->prev = xd;
            tmp->next = NULL;
            
            xd->next = tmp;
            hl->tail = tmp;
        }
        else {
            xd->prev->next = tmp;
            xd->prev = tmp;
            
            tmp->prev = xd->prev;
            tmp->next = xd;
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
    
    return list_add(l, &node, -1);
}




int list_remove(handle_t l, int index)
{
    list_node_t *xd;
    list_t *hl=(list_t*)l;
    
    if (!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    if (hl->size==0 || index>=hl->size) {
        lock_dynamic_release(hl->lock);
        return -1;
    }
    
    xd = node_at(hl, index);
    
    if(index==0) {                  //the head
        hl->head = xd->next;
        if(hl->head) {
            hl->head->prev = NULL;
        }
    }
    else if(index==hl->size-1 || index==-1) {    //the tail
        hl->tail = xd->prev;
        if(hl->tail) {
            hl->tail->next = NULL;
        }
    }
    else {
        
        xd->prev->next = xd->next;
        xd->next->prev = xd->prev; 
    }
    node_free(hl, xd); hl->size--;
    lock_dynamic_release(hl->lock);
    
    return 0;
}


int list_iterator(handle_t l, node_t *node, list_callback_t callback)
{
    int r;
    list_node_t *nd;
    list_t *hl=(list_t*)l;

    if (!hl) {

        return -1;
    }

    lock_dynamic_hold(hl->lock);
    if (hl->size==0) {
        lock_dynamic_release(hl->lock);
        return -1;
    }
    
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
    lock_dynamic_release(hl->lock);
    
    return NULL;
}

