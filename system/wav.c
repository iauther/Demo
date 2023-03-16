#include "wav.h"







static void get_timestamp(timestamp_t *ts)
{
    
}



wav_pkt_t* wav_init(U32 total_data_len)
{
    wav_pkt_t *p=calloc(1, sizeof(wav_data_t)*CHMAX+total_data_len);
    if(!p) {
        return NULL;
    }
    p->magic = WAV_MAGIC;
    p->len = 0;
    
    return p;
}


int wav_deinit(wav_pkt_t *p)
{
    wav_pkt_t **wp=(wav_pkt_t**)p;
    
    if(!wp || !(*wp)) {
        return -1;
    }
    free(*wp);
    
    return 0;
}


int wav_pack(wav_pkt_t *p, U8 ch, F32 *data, U32 len)
{
    wav_pkt_t *wp=(wav_pkt_t*)p;
    wav_data_t *wd=(wav_data_t*)(wp->data+wp->len);
    
    if(wp->magic != WAV_MAGIC) {
        wp->magic = WAV_MAGIC;
        wp->len = 0;
    }
    
    wd->ch  = ch;
    wd->len = len+sizeof(wav_data_t);
    get_timestamp(&wd->ts);
    memcpy(wd->data, data, len);
    wp->len  += wd->len;
    return 0;
}


int wav_unpack(wav_pkt_t *p, U8 idx, wav_data_t *wd)
{
    U8 i;
    U32 tlen=0,maxlen;
    wav_pkt_t *wp=(wav_pkt_t*)p;
    wav_data_t *tmp=NULL;

    if(wp->magic != WAV_MAGIC) {
        return -1;
    }

    maxlen=wp->len;
    for(i=0; i<idx; i++) {
        tmp = wp->data+tlen;
        tlen += tmp->len;
    }

    if(tlen>maxlen) {
        return -1;
    }

    get_timestamp(&wd->ts);
    memcpy(wd, tmp, tmp->len);

    return 0;
}
