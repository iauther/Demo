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
    int     mode;
    
    handle_t lock;
}rbuf_handle_t;



handle_t rbuf_init(rbuf_cfg_t *cfg)
{
    rbuf_handle_t *h=(rbuf_handle_t*)calloc(1, sizeof(rbuf_handle_t));
    
    if(!cfg || !h) {
        return NULL;
    }
    
	h->pr   = 0;
	h->pw   = 0;
	h->dlen = 0;
	h->size = cfg->size;
    h->mode = cfg->mode;
	h->buf  = (U8*)cfg->buf;
    h->lock = lock_init();
	
	return h;
}


int rbuf_free(handle_t h)
{
    rbuf_handle_t *rh=(rbuf_handle_t*)h;
    
	if(!rh) {
        return -1;
    }
    
    lock_free(rh->lock);
    free(rh);
    
    return 0;
}


int rbuf_read(handle_t h, void *buf, int len, U8 shift)
{
    int rlen;
    rbuf_handle_t *rh=(rbuf_handle_t*)h;
    
	if(!rh || !buf || !len) {
        return -1;
    }
    
    lock_on(rh->lock);
    if(rh->dlen==0) {       //buffer is null
        lock_off(rh->lock);
        return 0;
    }
    
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
                memcpy((U8*)buf+l, rh->buf, rlen-l);
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
                memcpy((U8*)buf+l, rh->buf, rlen-l);
            }
        }
    }

    if (shift) {
        rh->pr = (rh->pr + rlen) % rh->size;        //���¶�ָ��
        rh->dlen -= rlen;
    }
    lock_off(rh->lock);

	return rlen;
}

static void read_shift(rbuf_handle_t *rh, int len)
{
    int xlen=(len>rh->dlen)?rh->dlen:len;
    rh->pr = (rh->pr + len) % rh->size;        //���¶�ָ��
    rh->dlen -= len;
}
int rbuf_read_shift(handle_t h, int len)
{
    rbuf_handle_t* rh = (rbuf_handle_t*)h;

    if (!rh || !len) {
        return -1;
    }

    lock_on(rh->lock);
    read_shift(rh, len);
    lock_off(rh->lock);

    return 0;
}

int rbuf_write(handle_t h, void *buf, int len)
{
    int wlen;
    rbuf_handle_t *rh=(rbuf_handle_t*)h;
    
	if(!rh || !buf || !len) {
        return -1;
    }

    lock_on(rh->lock);
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
                memcpy(rh->buf, (U8*)buf+l, wlen-l);
            }
        }
    }
    else {  //ֻ�ܲ���д��
        if(rh->mode==RBUF_FULL_FILO) {  //�������������
        
            wlen = rh->size-rh->dlen;
            if(rh->pw <= rh->pr) {
                memcpy(rh->buf+rh->pw, buf, wlen);
            }
            else {
                int l = rh->size-rh->pw;
                memcpy(rh->buf+rh->pw, buf, l);
                memcpy(rh->buf, (U8*)buf+l, wlen-l);
            }
        }
        else {                          //��ǰ��������ݣ����������������
            //
            if(len>=rh->size) {
                wlen = rh->size;
                rh->pr = rh->pw = 0;
                
                memcpy(rh->buf, (U8*)buf+len-wlen, wlen);
            }
            else {                      //�轫�������ݶ�һ���֣�������ȫ��д��
                
                int shiftlen=len+rh->dlen-rh->size;
                read_shift(rh, shiftlen);
                
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
                        memcpy(rh->buf, (U8*)buf+l, wlen-l);
                    }
                }
            }
        }
    }

    rh->pw = (rh->pw + wlen) % rh->size;        //����дָ��
    rh->dlen += wlen;
    lock_off(rh->lock);

	return wlen;
}


int rbuf_clear(handle_t h)
{
    int size;
    rbuf_handle_t* rh = (rbuf_handle_t*)h;

    if (!rh) {
        return -1;
    }

    lock_on(rh->lock);
    rh->pr = 0;
    rh->pw = 0;
    rh->dlen = 0;
    lock_off(rh->lock);

    return 0;
}


int rbuf_size(handle_t h)
{
    int size;
    rbuf_handle_t *rh=(rbuf_handle_t*)h;
    
	if(!rh) {
        return -1;
    }
    
    lock_on(rh->lock);
    size = rh->size;
    lock_off(rh->lock);

	return size;
}


int rbuf_get_dlen(handle_t h)
{
    int dlen;
    rbuf_handle_t *rh=(rbuf_handle_t*)h;
    
	if(!rh) {
        return -1;
    }
    
    lock_on(rh->lock);
    dlen = rh->dlen;
    lock_off(rh->lock);

	return dlen;
}


int rbuf_is_full(handle_t h)
{
    int r=0;
    rbuf_handle_t *rh=(rbuf_handle_t*)h;
    
	if(!rh) {
        return -1;
    }
    
    lock_on(rh->lock);
    r = (rh->dlen==rh->size)?1:0;
    lock_off(rh->lock);
    
    return r;
}






