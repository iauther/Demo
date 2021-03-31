#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "rbuf.h"
#include "lock.h"

typedef struct {
	int     pr;
	int     pw;
	int     dlen;
	int     size;
    U8      *buf;
    
    handle_t lock;
}rbuf_handle_t;



handle_t rbuf_init(void *buf, int size)
{
    rbuf_handle_t *h=calloc(1, sizeof(rbuf_handle_t));
    
    if(!h) {
        return NULL;
    }
    
	h->pr   = 0;
	h->pw   = 0;
	h->dlen = 0;
	h->size = size;
	h->buf  = buf;
    h->lock = lock_dynamic_new();
	
	return h;
}


int rbuf_free(handle_t *h)
{
    rbuf_handle_t **rh=(rbuf_handle_t**)h;
    
	if(!rh || !(*rh)) {
        return -1;
    }
    
    lock_dynamic_free(&(*rh)->lock);
    free(*rh);
    
    return 0;
}


int rbuf_read(handle_t h, U8 *buf, int len)
{
    int rlen;
    rbuf_handle_t *rh=(rbuf_handle_t*)h;
    
	if(!rh || !buf || !len) {
        return -1;
    }
    
    //buffer is null
    if(rh->dlen==0) {
        return 0;
    }
    
    lock_dynamic_hold(rh->lock);
    if(len<=rh->dlen) {     //�������࣬��ȡ���ȵ���Ҫ��ĳ���
        rlen = len;
        if(rh->pr <= rh->pw) {
            memcpy(buf, rh->buf+rh->pr, rlen);
        }
        else {
            int l = rh->size-rh->pr;
            if(rlen<=l) {
                memcpy(buf, rh->buf+rh->pr, rlen);
            }
            else {
                memcpy(buf, rh->buf+rh->pr, l);
                memcpy(buf+l, rh->buf, rlen-l);
            }
        } 
    }
    else {      //���ݲ���, ��ȫ������������������С��Ҫ�󳤶�
        rlen = rh->dlen;
        if(rh->pr < rh->pw) {
            memcpy(buf, rh->buf+rh->pr, rlen);
        }
        else {
            int l = rh->size-rh->pr;
            if(rlen<=l) {
                memcpy(buf, rh->buf+rh->pr, rlen);
            }
            else {
                memcpy(buf, rh->buf+rh->pr, l);
                memcpy(buf+l, rh->buf, rlen-l);
            }
        }
    }
    rh->pr = (rh->pr+rlen)%rh->size;        //���¶�ָ��
    rh->dlen -= rlen;
    lock_dynamic_release(rh->lock);

	return rlen;
}



int rbuf_write(handle_t h, U8 *buf, int len)
{
    int wlen;
    rbuf_handle_t *rh=(rbuf_handle_t*)h;
    
	if(!rh || !buf || !len) {
        return -1;
    }
    
    //buffer is full
    if(rh->dlen==rh->size) {
        return 0;
    }

    lock_dynamic_hold(rh->lock);
    if(len <= (rh->size-rh->dlen)) {    //��ȫ��д��
        wlen = len;
        if(rh->pw <= rh->pr) {      //дָ����ǰ��ֻ��д1��
            memcpy(rh->buf+rh->pw, buf, wlen);
        }
        else {
            int l = rh->size-rh->pw;
            if(wlen<=l) {   //дָ���ں󣬺����㹻��ֻ��д1��
                memcpy(rh->buf+rh->pw, buf, wlen);
            }
            else {  //дָ���ں󣬺���ʣ��ռ䲻�㣬��д2��
                memcpy(rh->buf+rh->pw, buf, l);
                memcpy(rh->buf, buf+l, wlen-l);
            }
        }
    }
    else {  //ֻ�ܲ���д�룬��д��buffer
        wlen = rh->size-rh->dlen;
        if(rh->pw <= rh->pr) {
            memcpy(rh->buf+rh->pw, buf, wlen);
        }
        else {
            int l = rh->size-rh->pw;
            memcpy(rh->buf+rh->pw, buf, l);
            memcpy(rh->buf, buf+l, wlen-l);
        } 
    }
    rh->pw = (rh->pw+wlen)%rh->size;        //����дָ��
    rh->dlen += wlen;
    lock_dynamic_release(rh->lock);

	return wlen;
}


int rbuf_get_size(handle_t h)
{
    int size;
    rbuf_handle_t *rh=(rbuf_handle_t*)h;
    
	if(!rh) {
        return -1;
    }
    
    lock_dynamic_hold(rh->lock);
    size = rh->size;
    lock_dynamic_release(rh->lock);

	return size;
}


int rbuf_get_dlen(handle_t h)
{
    int dlen;
    rbuf_handle_t *rh=(rbuf_handle_t*)h;
    
	if(!rh) {
        return -1;
    }
    
    lock_dynamic_hold(rh->lock);
    dlen = rh->dlen;
    lock_dynamic_release(rh->lock);

	return dlen;
}



