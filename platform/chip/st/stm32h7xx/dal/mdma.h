#ifndef __MDMA_Hx__
#define __MDMA_Hx__

#include "types.h"


typedef void (*mdma_callback_t)(MDMA_HandleTypeDef *hmdma, int id);



typedef struct {
    U16	    chs;                            //振动通道总数量
    U16     chX;                            //前8路振动通道数量
    U16     chY;                            //后8路振动通道数量
    U32*	pPingDMA_A;                     //DMA_A ping buf地址
    U32*	pPongDMA_A;                     //DMA_A pong buf地址
    U32*	pPingDMA_B;                     //DMA_B ping buf地址
    U32*	pPongDMA_B;                     //DMA_B pong buf地址
    U32*	pPingMDMA;                      //MDMA ping buf地址
    U32*	pPongMDMA;                      //MDMA pong buf地址
    U16	    pointCnt;                       //DMA ping/pong buf每通道采集点数，最大数为4096
    U16	    blockCnt;                       //MDMA对DMA ping/pong buf按采集通道分组，再按BufBlockNum块构成用于计算的ping/pong buf
    
    mdma_callback_t callback;               //回调函数，用于通知应用层
}mdma_cfg_t;


int mdma_init(void);
int mdma_deinit(void);
int mdma_config(mdma_cfg_t *cfg);
int mdma_start(void);
int mdma_stop(void);


#endif
