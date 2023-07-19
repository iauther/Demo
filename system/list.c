#include "list.h"
#include "lock.h"
#include "xmem.h"
#include "log.h"


typedef struct list_node {
    struct list_node    *prev;
    struct list_node    *next;
    node_t              data;
}list_node_t;

typedef struct {
    list_node_t         *head;
    list_node_t         *tail;
    int                 size;
  
    handle_t            lock;
    list_cfg_t          cfg;
    
    list_node_t         **free;
} list_t;


static list_node_t* node_from_free(list_t *l, int dlen)
{
    int i;
    list_node_t *xd=NULL;
    
    for(i=0; i<l->cfg.max; i++) {
        xd = l->free[i];
        if(xd && xd->data.buf && xd->data.blen>=dlen) {
            xd = l->free[i];
            l->free[i] = NULL;
            return xd;
        }
    }
    
    return NULL;
}
static int node_to_free(list_t *l, list_node_t *node)
{
    int i;
    
    for(i=0; i<l->cfg.max; i++) {
        if(l->free[i]==NULL) {
            l->free[i] = node;
            return 0;
        }
    }
    
    return -1;
}


static list_node_t *node_new(list_t *l, void *data, int len)
{
    list_node_t *node=NULL;
    
    node = node_from_free(l, len);
    if(!node) {
        node = eMalloc(sizeof(list_node_t));
        if (!node) return NULL;
        
        node->prev = NULL;
        node->next = NULL;
        
        node->data.buf = eMalloc(len);
        if(!node->data.buf) {
            xFree(node);
            return NULL;
        }
        node->data.blen = len;
    }
    
    memcpy(node->data.buf, data, len);
    node->data.dlen = len;

    return node;
}
static int node_free(list_t *l, list_node_t *node)
{
    int i;
    
    for(i=0; i<l->cfg.max; i++) {
        if(l->free[i]==node) {
            l->free[i] = NULL;
        }
    }
    
    xFree(node->data.buf);
    xFree(node);
    
    return 0;
}


static int node_remove(list_t *l, list_node_t *node)
{
    int index;
    list_node_t *xd=node;
    
    if(xd==l->head) {
        if(l->size==1) {
            l->head = NULL;
            l->tail = NULL;
        }
        else {
            l->head = xd->next;
            if(l->head) {
                l->head->prev = NULL;
            }
        }
    }
    else if(xd==l->tail) {
        if(l->size==1) {
            l->head = NULL;
            l->tail = NULL;
        }
        else {
            l->tail = xd->prev;
            if(l->tail) {
                l->tail->next = NULL;
            }
        }
    }
    else {
        xd->prev->next = xd->next;
        xd->next->prev = xd->prev;
    }
    
    node_to_free(l, xd);
    l->size--;
    
    return 0;
}

static int node_delete(list_t *l, list_node_t *node)
{
    int i;
    
    int index;
    list_node_t *xd=node;
    
    if(xd==l->head) {
        if(l->size==1) {
            l->head = NULL;
            l->tail = NULL;
        }
        else {
            l->head = xd->next;
            if(l->head) {
                l->head->prev = NULL;
            }
        }
    }
    else if(xd==l->tail) {
        if(l->size==1) {
            l->head = NULL;
            l->tail = NULL;
        }
        else {
            l->tail = xd->prev;
            if(l->tail) {
                l->tail->next = NULL;
            }
        }
    }
    else {
        xd->prev->next = xd->next;
        xd->next->prev = xd->prev;
    }
    
    node_free(l, xd);
    l->size--;
    
    return -1;
}

static int node_set(list_t *l, list_node_t *node, node_t *nd)
{
    if(!node) {
        return -1;
    }
    
    xFree(node->data.buf);
    node->data.buf = eMalloc(nd->dlen);
    if(!node->data.buf) {
        return -1;
    }
    
    memcpy(node->data.buf, nd->buf, nd->dlen);
    node->data.blen = nd->dlen;
    node->data.dlen = nd->dlen;
    
    return 0;
}



static list_node_t *node_get(list_t *l, int index)
{
    if(index==0) {
        return l->head;
    }
    else if(index==l->size-1) {
        return l->tail;
    }
    else if (index<0 || index>=l->size) {
        return NULL;
    }
    else {
        int idx=0;
        list_node_t *tmp=l->head;
        
        while(tmp) {
            if(idx==index) {
                return tmp;
            }
            tmp = tmp->next;
            idx++;
        }
    }
    
    return NULL;
}



///////////////////////////////////////////////////////////
handle_t list_init(list_cfg_t *cfg)
{
    list_t *l=NULL;
    
    if (!cfg)
        return NULL;
    
    l = eMalloc(sizeof(list_t));
    if (!l)
        return NULL;
    
    l->head = NULL;
    l->tail = NULL;
    l->size = 0;
    l->lock = lock_dynamic_new();
    l->cfg = *cfg;
    
    l->free = (list_node_t**)eMalloc(sizeof(list_node_t)*l->cfg.max);
    
    return l;
}


int list_free(handle_t l)
{
    int i;
    list_node_t *xd;
    list_t *hl=(list_t*)l;
    
    if(!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    xd = hl->head;
    while(xd) {
        node_free(hl, xd);
        xd = xd->next;
    }
    for(i=0; i<hl->cfg.max; i++) {
        xd = hl->free[i];
        if(xd) {
            node_free(hl, xd);
        }
    }
    lock_dynamic_release(hl->lock);
    
    lock_dynamic_free(hl->lock);
    free(hl->free);
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
    
    nd = node_get(hl, index);
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
    
    nd = node_get(hl, index);
    if(nd) {
        node_set(hl, nd, node);
    }
    lock_dynamic_release(hl->lock);
    
    return nd?0:-1;
}



int list_addto(handle_t l, node_t *node, int index)
{
    int idx=0;
    list_t *hl=(list_t*)l;
    list_node_t *xd,*tmp;
    
    if (!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    if (index>hl->size) {
        LOGE("list_addto, invalid index, size: %d, index: %d\n", hl->size, index);
        lock_dynamic_release(hl->lock);
        return -1;
    }
    
    if (hl->size>=hl->cfg.max) {
        
        if(hl->cfg.mode==MODE_FILO) {
            lock_dynamic_release(hl->lock);
            return 0;
        }
        else {
            xd = node_get(hl, 0);
            node_remove(hl, xd);
        }
    }
    
    tmp = node_new(hl, node->buf, node->dlen);
    if(!tmp) {
        LOGE("list_addto, malloc %d error, l: 0x%08x\n", node->dlen, (U32)hl);
        lock_dynamic_release(hl->lock);
        return -1;
    }
    
    if(hl->size==0) {
        
        tmp->prev = NULL;
        tmp->next = NULL;
        
        hl->head = tmp;
        hl->tail = tmp;
    }
    else {
    
        if(index==0) {                  //head
            tmp->prev = NULL;
            tmp->next = hl->head;
            
            xd->prev = tmp;
            hl->head = tmp;
        }
        else if((index==(hl->size)) || (index==-1)) {   //tail
            
            hl->tail->next = tmp;
            
            tmp->prev = hl->tail;
            tmp->next = NULL;
            
            hl->tail = tmp;
        }
        else {
            
            xd = node_get(l, index);
            
            xd->prev->next = tmp;
            xd->prev = tmp;
            
            tmp->prev = xd->prev;
            tmp->next = xd;
        }
    }
    hl->size++;
        
    lock_dynamic_release(hl->lock);
    
    return 0;
}


int list_append(handle_t l, void *data, U32 len)
{
    node_t node={data, len, len};
    
    return list_addto(l, &node, -1);
}


int list_remove(handle_t l, int index)
{
    int r;
    list_node_t *xd;
    list_t *hl=(list_t*)l;
    
    if (!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    xd = node_get(l, index);
    r = node_remove(hl, xd);
    lock_dynamic_release(hl->lock);
    
    return r;
}



int list_delete(handle_t l, int index)
{
    int r;
    list_node_t *xd;
    list_t *hl=(list_t*)l;
    
    if (!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    xd = node_get(l, index);
    r = node_free(hl, xd);
    lock_dynamic_release(hl->lock);
    
    return r;
}



int list_size(handle_t l)
{
    int size=0;
    list_t *hl=(list_t*)l;
    
    if (!hl) {
        return -1;
    }
    
    lock_dynamic_hold(hl->lock);
    size = hl->size;
    lock_dynamic_release(hl->lock);
    
    return size;
}

int list_iterator(handle_t l, node_t *node, list_callback_t callback, void *arg)
{
    int r=0,act=0;
    list_node_t *xd=NULL;
    list_t *hl=(list_t*)l;

    if (!hl) {
        return -1;
    }

    lock_dynamic_hold(hl->lock);
    if (hl->size==0) {
        lock_dynamic_release(hl->lock);
        return -1;
    }
    
    xd = hl->head;
    while (xd) {
        if (callback) {
            r = callback(hl, node, &xd->data, arg, &act);
            if(act==ACT_STOP) {
                break;
            }
            else if(act==ACT_REMOVE) {
                node_remove(l, xd);
            }
            else if(act==ACT_DELETE) {
                node_delete(l, xd);
            }
        }
        
        xd = xd->next;
    }
    lock_dynamic_release(hl->lock);
    
    return r;
}

