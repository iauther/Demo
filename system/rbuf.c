#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "rbuf.h"
#include "lock.h"


int rbuf_init(rbuf_t *rb, void *buf, int size)
{
    if(!rb) {
        return -1;
    }
    
	rb->pr   = 0;
	rb->pw   = 0;
	rb->dlen = 0;
	rb->size = size;
	rb->buf  = buf;
	
	return 0;
}


int rbuf_free(rbuf_t *rb)
{
	if(!rb) {
        return -1;
    }
    
    rb->buf = NULL;
    rb->dlen = 0;
    
    return 0;
}


int rbuf_read(rbuf_t *rb, U8 *buf, int len)
{
    int rlen;
    
	if(!rb || !buf || !len) {
        return -1;
    }
    
    //buffer is null
    if(rb->dlen==0) {
        return 0;
    }
    
    if(len<=rb->dlen) {     //数据有余，读取长度等于要求的长度
        
        rlen = len;
        if(rb->pr <= rb->pw) {
            memcpy(buf, rb->buf+rb->pr, rlen);
        }
        else {
            int l = rb->size-rb->pr;
            if(rlen<=l) {
                memcpy(buf, rb->buf+rb->pr, rlen);
            }
            else {
                memcpy(buf, rb->buf+rb->pr, l);
                memcpy(buf+l, rb->buf, rlen-l);
            }
        } 
    }
    else {      //数据不足, 将全部读出，但读出长度小于要求长度
        rlen = rb->dlen;
        if(rb->pr < rb->pw) {
            memcpy(buf, rb->buf+rb->pr, rlen);
        }
        else {
            int l = rb->size-rb->pr;
            if(rlen<=l) {
                memcpy(buf, rb->buf+rb->pr, rlen);
            }
            else {
                memcpy(buf, rb->buf+rb->pr, l);
                memcpy(buf+l, rb->buf, rlen-l);
            }
        }
    }
    rb->pr = (rb->pr+rlen)%rb->size;        //更新读指针
    rb->dlen -= rlen;

	return rlen;
}



int rbuf_write(rbuf_t *rb, U8 *buf, int len)
{
    int wlen;
    
	if(!rb || !buf || !len) {
        return -1;
    }

    //buffer is full
    if(rb->dlen==rb->size) {
        return 0;
    }

    if(len <= (rb->size-rb->dlen)) {    //可全部写入
        wlen = len;
        if(rb->pw <= rb->pr) {      //写指针在前，只用写1次
            memcpy(rb->buf+rb->pw, buf, wlen);
        }
        else {
            int l = rb->size-rb->pw;
            if(wlen<=l) {   //写指针在后，后续足够，只需写1次
                memcpy(rb->buf+rb->pw, buf, wlen);
            }
            else {  //写指针在后，后续剩余空间不足，需写2次
                memcpy(rb->buf+rb->pw, buf, l);
                memcpy(rb->buf, buf+l, wlen-l);
            }
        }
    }
    else {  //只能部分写入，即写满buffer
        wlen = rb->size-rb->dlen;
        if(rb->pw <= rb->pr) {
            memcpy(rb->buf+rb->pw, buf, wlen);
        }
        else {
            int l = rb->size-rb->pw;
            memcpy(rb->buf+rb->pw, buf, l);
            memcpy(rb->buf, buf+l, wlen-l);
        } 
    }
    rb->pw = (rb->pw+wlen)%rb->size;        //更新写指针
    rb->dlen += wlen;

	return wlen;
}


int rbuf_get_size(rbuf_t *rb)
{
	if(!rb) {
        return -1;
    }

	return rb->size;
}


int rbuf_get_dlen(rbuf_t *rb)
{
	if(!rb) {
        return -1;
    }

	return rb->dlen;
}



