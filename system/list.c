#include "list.h"
#include "lock.h"
#include "mem.h"
#include "log.h"


typedef struct {
    list_node_t         *head;
    list_node_t         *tail;
    int                 size;
}dlist_t;


typedef struct {
    dlist_t             used;
  
    handle_t            lock;
    list_cfg_t          cfg;
    
    dlist_t             free;
}list_t;


static list_node_t* node_from_free(list_t *l, int dlen)
{
    int i;
    list_node_t *tmp=NULL;
    dlist_t *dl=&l->free;
    
    tmp = dl->head;         //从头部开始搜索合适节点
    while(tmp) {
        if(tmp->data.buf && tmp->data.blen>=dlen) {
            
            if(tmp==dl->head) {            //head
                if(dl->size==1) {
                    dl->head = NULL;
                    dl->tail = NULL;
                }
                else {
                    tmp->next->prev = NULL;
                    dl->head = tmp->next;
                }
            }
            else if(tmp==dl->tail) {
                if(dl->size==1) {
                    dl->head = NULL;
                    dl->tail = NULL;
                }
                else {
                    tmp->next->next = NULL;
                    dl->tail = tmp->next;
                }
            }
            else {
                tmp->prev->next = tmp->next;
                tmp->next->prev = tmp->prev;
            }
            
            dl->size--;
            
            return tmp;
        }
        tmp = tmp->next;
    }
    
    return NULL;
}
static int node_to_free(list_t *l, list_node_t *node)
{
    int r=-1;
    list_node_t *tail=NULL;
    dlist_t *dl=&l->free;
    
    tail = dl->tail;          //新节点加在free列表尾部
    if(tail==NULL) {          //无节点
        node->prev = NULL;
        node->next = NULL;
        
        dl->head = node;
        dl->tail = node;
    }
    else {
        
        node->prev = tail;
        node->next = NULL;
        
        tail->next = node;
        dl->tail = node;
        if(dl->head==NULL) {
            dl->head = node;
        }
    }
    dl->size++;
    
    return 0;
}


static list_node_t *node_new(list_t *l, node_t *n)
{
    list_node_t *node=NULL;
    
    node = node_from_free(l, n->dlen);
    if(!node) {
        node = eMalloc(sizeof(list_node_t));
        if (!node) return NULL;
        
        node->prev = NULL;
        node->next = NULL;
        
        node->data.buf = eMalloc(n->dlen);
        if(!node->data.buf) {
            xFree(node);
            return NULL;
        }
        node->data.blen = n->dlen;
    }
    
    memcpy(node->data.buf, n->buf, n->dlen);
    node->data.dlen = n->dlen;
    node->data.tp = n->tp;

    return node;
}
static int node_free(list_t *l, list_node_t *node)
{
    xFree(node->data.buf);
    xFree(node);
    
    return 0;
}


static int node_remove(list_t *l, list_node_t *node, U8 ds)
{
    int index;
    list_node_t *tmp=node;
    dlist_t *dl=&l->used;
    
    if(tmp==dl->head) {
        if(dl->size==1) {
            dl->head = NULL;
            dl->tail = NULL;
        }
        else {
            dl->head = tmp->next;
            dl->head->prev = NULL;
        }
    }
    else if(tmp==dl->tail) {
        if(dl->size==1) {
            dl->head = NULL;
            dl->tail = NULL;
        }
        else {
            dl->tail = tmp->prev;
            if(dl->tail) {
                dl->tail->next = NULL;
            }
        }
    }
    else {
        tmp->prev->next = tmp->next;
        tmp->next->prev = tmp->prev;
    }
    dl->size--;
    
    if(ds==1) {
        node_to_free(l, tmp);
    }
    else if(ds==2) {
        node_free(l, tmp);
    }
    
    return 0;
}

static int node_set(list_t *l, list_node_t *node, node_t *nd)
{
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
    int idx=0;
    dlist_t *dl=&l->used;
    list_node_t *tmp=dl->head;
    
    if(index==0) {
        return dl->head;
    }
    else if(index==dl->size-1) {
        return dl->tail;
    }
    else {
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
static int node_swap(list_node_t *n1, list_node_t *n2)
{
    if(!n1 || !n2) {
        return -1;;
    }
    
    node_t x=n1->data;
    n1->data = n2->data;
    n2->data = x;
    
    return 0;
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
    
    l->used.head = NULL;
    l->used.tail = NULL;
    l->used.size = 0;
    l->lock = lock_init();
    l->cfg = *cfg;
    
    l->free.head = NULL;
    l->free.tail = NULL;
    l->free.size = 0;
    
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
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_free lock failed\n");
        return -1;
    }
    
    xd = hl->used.head;
    while(xd) {
        node_free(hl, xd);
        xd = xd->next;
    }
    
    xd = hl->free.head;
    while(xd) {
        node_free(hl, xd);
        xd = xd->next;
    }
    lock_off(hl->lock);
    
    lock_free(hl->lock);
    free(hl->free.head);
    free(hl);
    
    return 0;
}


int list_get_node(handle_t l, list_node_t **lnode, int index)
{
    int r=-1;
    list_node_t *ln=NULL;
    list_t *hl=(list_t*)l;
    
    if(!hl || !lnode || index<0) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_get lock failed\n");
        return -1;
    }
    
    if (hl->used.size<=0 || index>=hl->used.size) {
        lock_off(hl->lock);
        return -1;
    }
    
    ln = node_get(hl, index);
    if(ln && lnode) {
        (*lnode) = ln;
        r = 0;
    }
    lock_off(hl->lock);
    
    return r;
}


int list_set_node(handle_t l, list_node_t *lnode, int index)
{
    int r=-1;
    list_node_t *ln=NULL;
    list_t *hl=(list_t*)l;
    
    if(!hl || !lnode || index<0) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_set lock failed\n");
        return -1;
    }
    
    if (hl->used.size<=0 || index>=hl->used.size) {
        lock_off(hl->lock);
        return -1;
    }
    
    ln = node_get(hl, index);
    if(ln) {
        node_set(hl, ln, &lnode->data);
        r = 0;
    }
    lock_off(hl->lock);
    
    return r;
}


int list_take_node(handle_t l, list_node_t **lnode, int index)
{
    list_node_t *ln=NULL;
    list_t *hl=(list_t*)l;
    
    if(!hl || !lnode || index<0) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_take_node lock failed\n");
        return -1;
    }
    
    if (hl->used.size<=0 || index>=hl->used.size) {
        lock_off(hl->lock);
        return -1;
    }
    
    ln = node_get(hl, index);
    if(!ln) {
        lock_off(hl->lock);
        return -1;
    }
    *lnode = ln;
    node_remove(hl, ln, 0);
    
    lock_off(hl->lock);    
    
    return 0;
}


int list_back_node(handle_t l, list_node_t *lnode)
{
    list_t *hl=(list_t*)l;
    
    if(!hl || !lnode) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_back_node lock failed\n");
        return -1;
    }
    
    node_to_free(l, lnode);
    lock_off(hl->lock);
    
    return 0;
}


int list_discard_node(handle_t l, list_node_t *lnode)
{
    list_t *hl=(list_t*)l;
    
    if(!hl || !lnode) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_discard_node lock failed\n");
        return -1;
    }
    node_free(l, lnode);
    
    lock_off(hl->lock);
    
    return 0;
}


int list_addto(handle_t l, node_t *node, int index)
{
    int idx=0;
    list_t *hl=(list_t*)l;
    list_node_t *xd,*tmp;
    dlist_t *dl=NULL;
    
    if(!hl || !node) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_addto lock failed\n");
        return -1;
    }
    
    dl = &hl->used;
    if (index>dl->size) {
        if (hl->cfg.log) LOGE("list_addto, invalid index, size: %d, index: %d\n", dl->size, index);
        lock_off(hl->lock);
        return -1;
    }
    
    if (hl->cfg.max>0 && dl->size>=hl->cfg.max) {
        
        if(hl->cfg.mode==LIST_FULL_FILO) {
            //LOGW("___ list is full, FILO mode, new data is discard\n");
            lock_off(hl->lock);
            return 0;
        }
        else {
            xd = node_get(hl, 0);
            node_remove(hl, xd, 1);
            //LOGW("___ list is full, size: %d, FIFO mode, old data is discard\n", dl->size);
        }
    }
    
    tmp = node_new(hl, node);
    if(!tmp) {
        if (hl->cfg.log) LOGE("list_addto, malloc %d error, l: 0x%08x, used.size: %d, free.size: %d\n", node->dlen, (U32)hl, hl->used.size, hl->free.size);
        lock_off(hl->lock);
        return -1;
    }
    
    if(dl->size==0) {
        tmp->prev = NULL;
        tmp->next = NULL;
        
        dl->head = tmp;
        dl->tail = tmp;
    }
    else {
    
        if(index==0) {                                  //head
            tmp->prev = NULL;
            tmp->next = dl->head;
            
            xd->prev = tmp;
            dl->head = tmp;
        }
        else if((index==(dl->size)) || (index==-1)) {   //tail
            
            tmp->prev = dl->tail;
            tmp->next = NULL;
            
            dl->tail->next = tmp;
            dl->tail = tmp;
        }
        else {
            
            xd = node_get(l, index);
            
            tmp->prev = xd->prev;
            tmp->next = xd;
            
            xd->prev->next = tmp;
            xd->prev = tmp;
        }
    }
    dl->size++;
        
    lock_off(hl->lock);
    
    return 0;
}


int list_add_node(handle_t l, list_node_t *lnode, int index)
{
    int idx=0;
    list_t *hl=(list_t*)l;
    list_node_t *xd,*tmp=lnode;
    dlist_t *dl=NULL;
    
    if(!hl || !lnode) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_addto lock failed\n");
        return -1;
    }
    
    dl = &hl->used;
    if (index>dl->size) {
        if (hl->cfg.log) LOGE("list_addto, invalid index, size: %d, index: %d\n", dl->size, index);
        lock_off(hl->lock);
        return -1;
    }
    
    if (hl->cfg.max>0 && dl->size>=hl->cfg.max) {
        
        if(hl->cfg.mode==LIST_FULL_FILO) {
            //LOGW("___ list is full, FILO mode, new data is discard\n");
            lock_off(hl->lock);
            return 0;
        }
        else {
            xd = node_get(hl, 0);
            node_remove(hl, xd, 1);
            //LOGW("___ list is full, size: %d, FIFO mode, old data is discard\n", dl->size);
        }
    }
    
    if(dl->size==0) {
        tmp->prev = NULL;
        tmp->next = NULL;
        
        dl->head = tmp;
        dl->tail = tmp;
    }
    else {
    
        if(index==0) {                                  //head
            tmp->prev = NULL;
            tmp->next = dl->head;
            
            xd->prev = tmp;
            dl->head = tmp;
        }
        else if((index==(dl->size)) || (index==-1)) {   //tail
            
            tmp->prev = dl->tail;
            tmp->next = NULL;
            
            dl->tail->next = tmp;
            dl->tail = tmp;
        }
        else {
            
            xd = node_get(l, index);
            
            tmp->prev = xd->prev;
            tmp->next = xd;
            
            xd->prev->next = tmp;
            xd->prev = tmp;
        }
    }
    dl->size++;
        
    lock_off(hl->lock);
    
    return 0;
}


int list_append(handle_t l, U32 tp, void *data, U32 len)
{
    node_t node={tp, data, len, len};
    
    return list_addto(l, &node, -1);
}


int list_append_node(handle_t l, list_node_t *lnode)
{
    return list_add_node(l, lnode, -1);
}


int list_remove(handle_t l, int index)
{
    int r;
    list_node_t *xd;
    list_t *hl=(list_t*)l;
    
    if (!hl) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_remove lock failed\n");
        return -1;
    }
    
    xd = node_get(l, index);
    if(!xd) {
        lock_off(hl->lock);
        return -1;
    }
    
    r = node_remove(hl, xd, 1);
    lock_off(hl->lock);
    
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
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_delete lock failed\n");
        return -1;
    }
    
    if(hl->used.size<=0 || index>=hl->used.size) {
        lock_off(hl->lock);
        return -1;
    }
    
    xd = node_get(l, index);
    r = node_free(hl, xd);
    lock_off(hl->lock);
    
    return r;
}


int list_sort(handle_t l, U8 order)
{
    int size=0;
    list_t *hl=(list_t*)l;
    
    if (!hl || order>=SORT_MAX) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_size lock failed\n");
        return -1;
    }
    
    if(hl->used.size<=2) {
        lock_off(hl->lock);
        return -1;
    }
    
    {
        int len,cmp;
        dlist_t *dl=&hl->used;
        list_node_t *p2,*p1=dl->head;
        
        while(p1) {
            p2 = p1->next;
            while(p2) {
                len = MIN(p1->data.dlen, p2->data.dlen);
                cmp = memcmp(p1->data.buf, p2->data.buf, len);
                if(((cmp>0) && (order==SORT_ASCEND)) || ((cmp<0) && (order==SORT_DESCEND))) {
                    node_swap(p1, p2);
                }
                
                p2 = p2->next;
            }
            p1 = p1->next;
        }  
    }
    
    lock_off(hl->lock);
    
    return 0;
}


int list_size(handle_t l)
{
    int size=0;
    list_t *hl=(list_t*)l;
    
    if (!hl) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_size lock failed\n");
        return -1;
    }
    
    size = hl->used.size;
    lock_off(hl->lock);
    
    return size;
}


int list_clear(handle_t l)
{
    list_node_t *xd;
    list_t *hl=(list_t*)l;
    
    if (!hl) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_clear lock failed\n");
        return -1;
    }
    
    if(hl->used.size<=0) {
        lock_off(hl->lock);
        return -1;
    }
    
    xd = hl->used.head;
    while(xd) {
        node_remove(hl, xd, 1);
        xd = xd->next;
    }
    hl->used.size = 0;
    hl->used.head = NULL;
    hl->used.tail = NULL;
    
    lock_off(hl->lock);
    
    return 0;
}



int list_iterator(handle_t l, node_t *node, list_callback_t callback, void *arg)
{
    int r=0,act=0;
    list_node_t *xd=NULL;
    list_t *hl=(list_t*)l;
    dlist_t *dl=NULL;

    if (!hl) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_iterator lock failed\n");
        return -1;
    }
    
    dl = &hl->used;
    if (dl->size<=0) {
        lock_off(hl->lock);
        return -1;
    }
    
    xd = dl->head;
    while (xd) {
        if (callback) {
            r = callback(hl, node, &xd->data, arg, &act);
            if(act==ACT_STOP) {
                break;
            }
            else if(act==ACT_REMOVE) {
                node_remove(l, xd, 1);
            }
            else if(act==ACT_DELETE) {
                node_remove(l, xd, 2);
            }
        }
        
        xd = xd->next;
    }
    lock_off(hl->lock);
    
    return r;
}

