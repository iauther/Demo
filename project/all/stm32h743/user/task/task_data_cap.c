#include "task.h"
#include "dal_cap.h"
#include "dal_delay.h"
#include "log.h"
#include "list.h"
#include "cfg.h"
#include "json.h"

#include "fs.h"


/*
	PB6:    SEL1   PB7:  SEL0
    SEL0:   0: CH1 ON   1: CH2 ON
    SEL1:   0: CH3 ON   1: CH4 ON
*/
#define IS_CHANNEL(bits,ch) (bits&(1<<ch))
static void adc_switch(void)
{
#if ADC_MUX==1
    int delay=0;
    GPIO_PinState hl;
    U8 chBits=tasksHandle.chBits;
    
	if(IS_CHANNEL(chBits,CH_8)) {	                        //channel 1 on
        hl = GPIO_PIN_RESET;
	}
    else {	                                                //channel 2 on
        hl = GPIO_PIN_SET;
	}
    
    if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) != hl) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, hl);
        delay = 1;
    }
       
    ////////////////////////////////////////////////////
	if(IS_CHANNEL(chBits,CH_10)) {	                        //channel 3 on
        hl = GPIO_PIN_RESET;
	}
    else {	                                                //channel 4 on
        hl = GPIO_PIN_SET;
	}
    
    if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) != hl) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, hl);
        delay = 1;
    }
    
    if(delay>0) {
        dal_delay_ms(1);
    }
#endif
}
static void adc_switch_init(void)
{
#if ADC_MUX==1
	GPIO_InitTypeDef init = {0};
		
	__HAL_RCC_GPIOB_CLK_ENABLE();

	init.Pin = GPIO_PIN_6|GPIO_PIN_7;
	init.Mode = GPIO_MODE_OUTPUT_PP;
	init.Pull = GPIO_NOPULL;
	init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &init);
#endif
}

static void adc_data_fill(U8 dual, U32 *buf, int len)
{
    int i,r;
    static int flag=0;
    U16 *p16=(U16*)buf;
    int dlen=dual?(len):(len*2);
    int cnt=dual?(len/4):(len/2);
    tmp_buf_t *tb=&tasksHandle.capBuf;
    U32 *pch1=(U32*)tb->buf;
    U32 *pch2=(U32*)((U8*)tb->buf+dlen);
    
    for(i=0; i<cnt; i++) {
        if(dual) {
            pch1[i] = buf[i]&0xffff;
            pch2[i] = buf[i]>>16;
        }
        else {
            pch1[i] = p16[i];
            pch2[i] = p16[i+1];
        }
    }
    
    if(flag==0) {
        list_append(tasksHandle.oriList[CH_8], pch1, dlen);
        list_append(tasksHandle.oriList[CH_9], pch2, dlen);
    }
    else {
        list_append(tasksHandle.oriList[CH_10], pch1, dlen);
        list_append(tasksHandle.oriList[CH_11], pch2, dlen);
    }
    
    flag = !flag;
}



static void sai_data_fill(U32 *buf, int len)
{
    int i,offset=0,cl;
    U16 *ptr=(U16*)buf;
    U8 chs=tasksHandle.chs;
    U8 chBits=tasksHandle.chBits;

    cl = len/chs;
    for(i=0; i<8; i++) {
        if(chBits&(1<<i)) {
            list_append(tasksHandle.oriList[i], buf+offset, cl);
            offset += cl;
        }
    }
}


#ifdef OS_KERNEL
static void adc_data_callback(U8 *data, U32 len)
{
    node_t node={data,len,len};

    task_post(TASK_DATA_CAP, NULL, EVT_ADC, 0, &node, sizeof(node));
}
static void sai_data_callback(U8 *data, U32 len)
{
    node_t node={data,len,len};

    task_post(TASK_DATA_CAP, NULL, EVT_SAI, 0, &node, sizeof(node));
}
static void tmr_data_callback(U8 *data, U32 len)
{
    node_t node={data,len,len};
    
    //task_post(TASK_DATA_CAP, NULL, EVT_TMR, 0, &node, sizeof(node));
}


static void ccfg_to_dcfg(dal_cap_cfg_t *cfg)
{
    U8 i;
    
    tasksHandle.chs = 0;
    tasksHandle.chBits = 0;
    for(i=0; i<8; i++) {
        if(tasksHandle.cfg[i].chID<CH_MAX) {
            tasksHandle.chBits |= (1<<i);
            tasksHandle.chs++;
        }
    } 
}
void cap_config(void)
{
    dal_cap_cfg_t cfg;
    
    adc_switch_init();
    ccfg_to_dcfg(&cfg);
    
    cfg.chs     = tasksHandle.chs;
    cfg.chBits  = tasksHandle.chBits;
    cfg.e3Mux   = ADC_MUX;
    cfg.adc_fn  = adc_data_callback;
    cfg.sai_fn  = sai_data_callback;
    cfg.tmr_fn  = tmr_data_callback;
    dal_cap_config(&cfg);
    
    
}

void task_data_cap_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;    
    
    LOGD("_____ task_data_cap running\n");
    while(1) {
        r = task_recv(TASK_DATA_CAP, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                case EVT_ADC:
                {
                    node_t *nd=(node_t*)e.data;
                    
                    //LOGD("___ EVT_ADC\n");
                    adc_data_fill(ADC_DUAL_MODE, nd->buf, nd->dlen);
                    r = task_post(TASK_DATA_PROC, NULL, EVT_DATA, 0, NULL, 0);
                    adc_switch();
                }
                break;
                
                case EVT_SAI:
                {
                    node_t *nd=(node_t*)e.data;
                    
                    //LOGD("___ EVT_SAI\n");
                    sai_data_fill(nd->buf, nd->dlen);
                    task_post(TASK_DATA_PROC, NULL, EVT_DATA, 0, NULL, 0);
                }
                break;
                
                case EVT_TMR:
                {
                    node_t *nd=(node_t*)e.data;
                    //
                }
                break;
            }
        }
    }
}
#endif














