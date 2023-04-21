#ifndef __DAL_MDMA_Hx__
#define __DAL_MDMA_Hx__

#include "types.h"


typedef void (*mdma_callback_t)(MDMA_HandleTypeDef *hmdma, int id);



typedef struct {
    U16	    chs;                            //��ͨ��������
    U16     chX;                            //ǰ8·��ͨ������
    U16     chY;                            //��8·��ͨ������
    U32*	pPingDMA_A;                     //DMA_A ping buf��ַ
    U32*	pPongDMA_A;                     //DMA_A pong buf��ַ
    U32*	pPingDMA_B;                     //DMA_B ping buf��ַ
    U32*	pPongDMA_B;                     //DMA_B pong buf��ַ
    U32*	pPingMDMA;                      //MDMA ping buf��ַ
    U32*	pPongMDMA;                      //MDMA pong buf��ַ
    U16	    pointCnt;                       //DMA ping/pong bufÿͨ���ɼ������������Ϊ4096
    U16	    blockCnt;                       //MDMA��DMA ping/pong buf���ɼ�ͨ�����飬�ٰ�BufBlockNum�鹹�����ڼ����ping/pong buf
    
    mdma_callback_t callback;               //�ص�����������֪ͨӦ�ò�
}mdma_cfg_t;


int dal_mdma_init(void);
int dal_mdma_deinit(void);
int dal_mdma_config(mdma_cfg_t *cfg);
int dal_mdma_start(void);
int dal_mdma_stop(void);


#endif
