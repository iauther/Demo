#include "dac.h"
#include "log.h"
#include "mem.h"
#include "dal_dac.h"

typedef struct {
    handle_t    hdac;
    dac_param_t para;
    
    U16         *buf;
    int         blen;
}dac_handle_t;


static dac_handle_t dacHandle={
    .hdac = NULL,
    .para = {
        .enable = 0,
        .fdiv   = 1,
    }
};


int dac_init(void)
{
    handle_t h;

    h = dal_dac_init(DAC_1);
    if(!h) {
        LOGE("___ dac init error\n");
        return -1;
    }
    dacHandle.hdac = h;
    
    return 0;
}


int dac_set(dac_param_t *para)
{
    int r;
    dal_dac_para_t dpara;
    
    if(!para) {
        return -1;
    }
    
    dacHandle.para = *para;
    dpara.freq = dacHandle.para.freq/dacHandle.para.fdiv;
    dpara.blen = dacHandle.para.points*sizeof(U16);
    dpara.buf  = eMalloc(dpara.blen);
    if(!dpara.buf) {
        LOGE("___ dac mallco failed\n");
        return -1;
    }
    
    if(dacHandle.buf) {
        xFree(dacHandle.buf);
    }
    dacHandle.buf = dpara.buf;
    dacHandle.blen = dpara.blen;
    
    r = dal_dac_set(dacHandle.hdac, &dpara);
    
    if(dacHandle.para.enable) {
        dal_dac_start(dacHandle.hdac);
    }
    else {
        dal_dac_stop(dacHandle.hdac);
    }
    
    return r;
}


int dac_data_fill(U16 *data, U32 cnt)
{
    U32 i,j;
    
    if(!dacHandle.para.enable || !dacHandle.hdac) {
        return -1;
    }
    
    if(!data || !cnt) {
        return -1;
    }
    
    for(i=0; i<cnt; i++) { 
        if(i%dacHandle.para.fdiv==0) {
            dacHandle.buf[j++] = (data[i]<0x8000)?(data[i]+0x8000):(data[i]-0x8000);
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






