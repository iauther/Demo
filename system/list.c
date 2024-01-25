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
static list_node_t *node_get(list_t *l, int index);

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
static int node_insert(list_t *l, list_node_t *node, int index)
{
    list_node_t *xd,*tmp=node;
    dlist_t *dl=&l->used;
    
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
        goto quit;
    }
    
    ln = node_get(hl, index);
    if(!ln) {
        goto quit;
    }
    
    *lnode = ln;
    r = 0;
    
quit:
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
        goto quit;
    }
    
    ln = node_get(hl, index);
    if(ln) {
        r = node_set(hl, ln, &lnode->data);
    }
    
quit:
    lock_off(hl->lock);    
    return r;
}


int list_take_node(handle_t l, list_node_t **lnode, int index)
{
    int r=-1;
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
       goto quit;
    }
    
    ln = node_get(hl, index);
    if(!ln) {
        goto quit;
    }
    *lnode = ln;
    r = node_remove(hl, ln, 0);

quit:
    lock_off(hl->lock);    
    return r;
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


int list_insert(handle_t l, node_t *node, int index)
{
    int r=-1;
    list_t *hl=(list_t*)l;
    list_node_t *xd,*tmp;
    
    if(!hl || !node) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___list_insert lock failed\n");
        return -1;
    }
    
    if (index>hl->used.size) {
        if (hl->cfg.log) LOGE("___list_insert, invalid index, size: %d, index: %d\n", hl->used.size, index);
        goto quit;
    }
    
    if (hl->cfg.max>0 && hl->used.size>=hl->cfg.max) {
        //链表装满时, 根据策略丢数据
        //如果要插入的位置为末尾, 则丢弃
        if(index<0 || index>=hl->used.size-1) {
            if(hl->cfg.mode==LIST_FULL_FILO) { //策略为丢新数据时
                if (hl->cfg.log) {
                    //LOGW("___list_insert, list full, discard the new data\n");
                }
                r = -2; goto quit;
            }
            
            if (hl->cfg.log) {
                //LOGW("___list_insert, list full, discard the old data\n");
            }
            
            xd = node_get(hl, 0);              //找最老的数据丢
            node_remove(hl, xd, 1);
        }
        else {  //如果插入位置位于中间, 则需丢最后一个数据
            if (hl->cfg.log) {
                LOGW("___list_insert, insert to the middle, discard the last data\n");
            }
            
            xd = node_get(hl, hl->used.size-1);
            node_remove(hl, xd, 1);
        }
    }
    
    tmp = node_new(hl, node);
    if(!tmp) {
        if (hl->cfg.log) LOGE("___list_insert, malloc %d error, l: 0x%08x, used.size: %d, free.size: %d\n", node->dlen, (U32)hl, hl->used.size, hl->free.size);
        goto quit;
    }
    r = node_insert(l, tmp, index);

quit:
    lock_off(hl->lock);
    
    return r;
}


int list_insert_node(handle_t l, list_node_t *lnode, int index)
{
    int r=-1;
    list_t *hl=(list_t*)l;
    list_node_t *xd,*tmp=lnode;
    
    if(!hl || !lnode) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_addto lock failed\n");
        return -1;
    }
    
    if (index>hl->used.size) {
        if (hl->cfg.log) LOGE("list_addto, invalid index, size: %d, index: %d\n", hl->used.size, index);
        goto quit;
    }
    
    if (hl->cfg.max>0 && hl->used.size>=hl->cfg.max) {
        //链表装满时, 根据策略丢数据
        //如果要插入的位置为末尾, 则丢弃
        if(index<0 || index>=hl->used.size-1) {
            if(hl->cfg.mode==LIST_FULL_FILO) { //策略为丢新数据时
                r = 0; goto quit;
            }
            xd = node_get(hl, 0);              //找最老的数据丢
            node_remove(hl, xd, 1);
        }
        else {  //如果插入位置位于中间, 则需丢最后一个数据
            xd = node_get(hl, hl->used.size-1);
            node_remove(hl, xd, 1);
        }
    }
    r = node_insert(l, lnode, index);

quit:
    lock_off(hl->lock);
    return r;
}


int list_append(handle_t l, U32 tp, void *data, U32 len)
{
    node_t node={tp, data, len, len};
    
    return list_insert(l, &node, -1);
}


int list_append_node(handle_t l, list_node_t *lnode)
{
    return list_insert_node(l, lnode, -1);
}


int list_infront(handle_t l, U32 tp, void *data, U32 len)
{
    node_t node={tp, data, len, len};
    
    return list_insert(l, &node, 0);
}


int list_infront_node(handle_t l, list_node_t *lnode)
{
    return list_insert_node(l, lnode, 0);
}


int list_remove(handle_t l, int index)
{
    int r=-1;
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
        goto quit;
    }
    r = node_remove(hl, xd, 1);
    
quit:
    lock_off(hl->lock);
    return r;
}



int list_delete(handle_t l, int index)
{
    int r=-1;
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
        goto quit;
    }
    
    xd = node_get(l, index);
    r = node_free(hl, xd);
    
quit:
    lock_off(hl->lock);
    return r;
}


int list_sort(handle_t l, U8 order)
{
    int r=-1;
    list_t *hl=(list_t*)l;
    
    if (!hl || order>=SORT_MAX) {
        return -1;
    }
    
    if(lock_on(hl->lock)) {
        if (hl->cfg.log) LOGE("___ list_sort lock failed\n");
        return -1;
    }
    
    if(hl->used.size<2) {
        goto quit;
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
    r = 0;
    
quit:
    lock_off(hl->lock);
    return r;
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
    int r=-1;
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
        goto quit;
    }
    
    xd = hl->used.head;
    while(xd) {
        node_remove(hl, xd, 1);
        xd = xd->next;
    }
    hl->used.size = 0;
    hl->used.head = NULL;
    hl->used.tail = NULL;
    r = 0;
    
quit:
    lock_off(hl->lock);
    return r;
}



int list_iterator(handle_t l, node_t *node, list_callback_t callback, void *arg)
{
    int r=-1,act=0;
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
        goto quit;
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
    r = 0;
    
quit:
    lock_off(hl->lock);
    return r;
}

