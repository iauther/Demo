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
    
    if(len<=rb->dlen) {     //�������࣬��ȡ���ȵ���Ҫ��ĳ���
        
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
    else {      //���ݲ���, ��ȫ������������������С��Ҫ�󳤶�
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
    rb->pr = (rb->pr+rlen)%rb->size;        //���¶�ָ��
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

    if(len <= (rb->size-rb->dlen)) {    //��ȫ��д��
        wlen = len;
        if(rb->pw <= rb->pr) {      //дָ����ǰ��ֻ��д1��
            memcpy(rb->buf+rb->pw, buf, wlen);
        }
        else {
            int l = rb->size-rb->pw;
            if(wlen<=l) {   //дָ���ں󣬺����㹻��ֻ��д1��
                memcpy(rb->buf+rb->pw, buf, wlen);
            }
            else {  //дָ���ں󣬺���ʣ��ռ䲻�㣬��д2��
                memcpy(rb->buf+rb->pw, buf, l);
                memcpy(rb->buf, buf+l, wlen-l);
            }
        }
    }
    else {  //ֻ�ܲ���д�룬��д��buffer
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
    rb->pw = (rb->pw+wlen)%rb->size;        //����дָ��
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



