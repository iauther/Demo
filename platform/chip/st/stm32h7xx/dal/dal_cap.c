#include "snd.h"
#include "log.h"
#include "cfg.h"
#include "dal_cap.h"

enum {
    BUF_PING=0,
    BUF_PONG,
};
typedef struct {
    U32 inuse;
    U32 cnt;
    
    U32 chs;
    U32 totalCnt;
    U32 totalLen;
}mdma_gbl_t;

typedef struct {
    dal_adc_cfg_t adc;
    dal_sai_cfg_t sai;
    dal_tmr_cfg_t tmr;
}sig_cfg_t;



#define BLOCK_CNT                       4
#define BLOCK_DATA_CNT                  (2000) // 每块数据长度
#define CHN_DATA_LEN                    (BLOCK_DATA_CNT*BLOCK_CNT)

#define SAI_PINGPONG_MAX_CNT_DMA	    (2000*8)												 
#define SAI_PINGPONG_MAX_CNT_MDMA	    (2000*16*4)  //4的语义是4次DMA传输完成	

U32 saiBufPingDMA_A[SAI_PINGPONG_MAX_CNT_DMA] __attribute__ ((at(0x30000000))); // SRAM1
U32 saiBufPongDMA_A[SAI_PINGPONG_MAX_CNT_DMA] __attribute__ ((at(0x30010000))); // SRAM1

U32 saiBufPingDMA_B[SAI_PINGPONG_MAX_CNT_DMA] __attribute__ ((at(0x30020000))); // SRAM2
U32 saiBufPongDMA_B[SAI_PINGPONG_MAX_CNT_DMA] __attribute__ ((at(0x30030000))); // SRAM2

U32 saiBufPingMDMA[SAI_PINGPONG_MAX_CNT_MDMA] __attribute__ ((at(0xD3200000))); // SDRAM
U32 saiBufPongMDMA[SAI_PINGPONG_MAX_CNT_MDMA] __attribute__ ((at(0xD3280000))); // SDRAM


static mdma_gbl_t gblMdma={0};
static dal_cap_cfg_t  capCfg={0};

/////////////////////////////////////////////////////////////////////
static U32* get_mdma_data(U32 *len)
{
    U32 *ptr=NULL;
    
    gblMdma.cnt++;
    if(gblMdma.cnt == (gblMdma.totalCnt)) {
        gblMdma.cnt = 0;
        if(gblMdma.inuse==BUF_PING) {
            ptr = saiBufPingMDMA;
            gblMdma.inuse = BUF_PONG;
        }
        else {
            ptr = saiBufPongMDMA;
            gblMdma.inuse = BUF_PING;
        }
        
        *len = gblMdma.totalLen;
    }
    
    return ptr;
}
static void mdma_data_callback(MDMA_HandleTypeDef *hmdma, int id)
{
    U32 dlen;
    U32 *pData=NULL;
    
    pData = get_mdma_data(&dlen);
    if(pData) {
        //LOGD("______ id: %d\n", id);
        if(capCfg.sai_fn) {
            capCfg.sai_fn((U8*)pData, dlen);
        }
    }
}


static void sig_init(void)
{
    dal_adc_init();
    snd_init();
    dal_sai_init();
    //dal_tmr_init();
}
static void sig_config(sig_cfg_t *cfg)
{
    dal_adc_config(&cfg->adc);
    dal_sai_config(&cfg->sai);
    //dal_tmr_config(&cfg->tmr);
}
static void sig_start(void)
{
    dal_adc_start();
    dal_sai_start();
    //dal_tmr_start();
}
static void sig_stop(void)
{
    dal_adc_stop();
    dal_sai_stop();
    //dal_tmr_stop();
}
static void sig_set(dal_cap_cfg_t *cfg)
{
    sig_cfg_t sc;
    
    capCfg = *cfg;
    sc.adc.dual = ADC_DUAL_MODE;
    sc.adc.mode = MODE_DMA;
    sc.adc.samples = ADC_SAMPLE_COUNT;
    sc.adc.callback = capCfg.adc_fn;

    sc.sai.e3Mux = 1;
    sc.sai.chBits = 0x00ff;
    
    sc.sai.mdma.chs = 8;
    sc.sai.mdma.chX = 8;
    sc.sai.mdma.chY = 0;
    sc.sai.mdma.pPingDMA_A = saiBufPingDMA_A;
    sc.sai.mdma.pPongDMA_A = saiBufPongDMA_A;
    sc.sai.mdma.pPingDMA_B = saiBufPingDMA_B;
    sc.sai.mdma.pPongDMA_B = saiBufPongDMA_B;
    sc.sai.mdma.pPingMDMA = saiBufPingMDMA;
    sc.sai.mdma.pPongMDMA = saiBufPongMDMA;
    sc.sai.mdma.pointCnt = BLOCK_DATA_CNT;
    sc.sai.mdma.blockCnt = BLOCK_CNT;
    
    gblMdma.chs = sc.sai.mdma.chs;
    gblMdma.inuse = BUF_PING;
    gblMdma.cnt   = 0;
    
    gblMdma.totalCnt = sc.sai.mdma.blockCnt;
    gblMdma.totalLen = gblMdma.chs*CHN_DATA_LEN;
    
    sc.sai.mdma.callback = mdma_data_callback;
    
    //cfg.tmr = 0;
    sig_config(&sc);
}
/////////////////////////////////////////////////////////////////////

int dal_cap_init(void)
{
    sig_init();
    
    return 0;
}


int dal_cap_config(dal_cap_cfg_t *cfg)
{
    sig_set(cfg);
    return 0;
}


int dal_cap_start(void)
{
    sig_start();
    
    return 0;
}


int dal_cap_stop(void)
{
    sig_stop();
    
    return 0;
}


