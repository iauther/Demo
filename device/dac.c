#include "dac.h"
#include "log.h"


typedef struct {
    handle_t    hdac;
    dac_param_t para;
}dac_handle_t;


dac_handle_t dacHandle={
    .hdac = NULL,
    .para = {
        .enable = 0,
        .fdiv   = 1,
    }
};


int dac_init(dac_param_t *para)
{
    handle_t h;
    dal_dac_para_t dpara;

    dpara.port = DAC_1;
    dpara.freq = para->freq/para->fdiv;
    dpara.blen = para->buf.blen;
    dpara.buf  = para->buf.buf;
    dpara.fn   = para->fn;
    
    h = dal_dac_init(&dpara);
    if(!h) {
        LOGE("___ dac init error\n");
        return -1;
    }
    dacHandle.para = *para;
    dacHandle.hdac = h;
    
    return 0;
}


int dac_set(dac_param_t *para)
{
    int r;
    dal_dac_para_t dpara;
    
    if(!para || !para->buf.buf || (para->buf.blen<=0)) {
        return -1;
    }
    
    dacHandle.para = *para;
    
    dpara.freq = para->freq/para->fdiv;
    dpara.buf  = para->buf.buf;
    dpara.blen = para->buf.blen;
    
    
    r = dal_dac_set(dacHandle.hdac, &dpara);
    
    return r;
}


int dac_data_fill(U16 *data, U32 cnt)
{
    U32 i,j=0;
    U16 *pbuf=NULL;
    
    if(!dacHandle.para.enable || !dacHandle.hdac) {
        return -1;
    }
    
    if(!data || !cnt) {
        return -1;
    }
    pbuf = (U16*)dacHandle.para.buf.buf;
    
    for(i=0; i<cnt; i++) { 
        if(i%dacHandle.para.fdiv==0) {
            pbuf[j++] = (data[i]<0x8000)?(data[i]+0x8000):(data[i]-0x8000);
        }
    }
    
    return 0;
}



int dac_start(void)
{
    if(!dacHandle.para.enable || !dacHandle.hdac) {
        return -1;
    }
    
    return dal_dac_start(dacHandle.hdac);
}


int dac_stop(void)
{
    if(!dacHandle.hdac) {
        return -1;
    }
    
    return dal_dac_stop(dacHandle.hdac);
}






