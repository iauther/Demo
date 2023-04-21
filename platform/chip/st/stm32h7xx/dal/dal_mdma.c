#include "dal_mdma.h"
#include "log.h"

static MDMA_HandleTypeDef    MDMA_Handle = {0};
ALIGN_32BYTES(MDMA_LinkNodeTypeDef Xfer_Node[128]) = {0};
static U32 dummy_source = 0;
static U32 dummy_destination = 0;
static mdma_callback_t mdma_callback=NULL;
void MDMA_RepBlockCpltCallback(MDMA_HandleTypeDef *hmdma)
{
	U8* pMDMABufAddr = NULL;
	
    if(mdma_callback) {
        mdma_callback(hmdma, 0);
    }
}

void MDMA_TransferErrorCallback(MDMA_HandleTypeDef *hmdma)
{
    //LOGE("********** MDMA Transfer Error!!!!! **********\r\n");
    if(mdma_callback) {
        mdma_callback(hmdma, 1);
    }
}
void MDMA_IRQHandler(void)
{
	HAL_MDMA_IRQHandler(&MDMA_Handle);
}



int dal_mdma_init(void)
{
    __HAL_RCC_MDMA_CLK_ENABLE();
    return 0;
}


int dal_mdma_deinit(void)
{
    __HAL_RCC_MDMA_CLK_DISABLE();
    return 0;
}



int dal_mdma_config(mdma_cfg_t *cfg)
{
    U8 u8NodeLoop; // 内层循环，用于对DMA ping或pong缓存的ADC各通道数据进行分组
    U8 u8BlockLoop; // 外层循环，最终实现和XXXDAQ一样的ping/pong buf
    U8 u8NodeIndex;
    MDMA_LinkNodeConfTypeDef mdmaLinkNodeConfig = {0};
    
    /*S1:Select the MDMA instance to be used for the transfer*/
    MDMA_Handle.Instance = MDMA_Channel0;
    
    if (!cfg || HAL_MDMA_DeInit(&MDMA_Handle) != HAL_OK)
    {
        return -1;
    }
    
    /*S2:Initialize the MDMA node 0 (Dummy node)*/
    MDMA_Handle.Init.Request              = MDMA_REQUEST_SW;
    MDMA_Handle.Init.TransferTriggerMode  = MDMA_BUFFER_TRANSFER;
    MDMA_Handle.Init.Priority             = MDMA_PRIORITY_HIGH;
    MDMA_Handle.Init.Endianness           = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    MDMA_Handle.Init.SourceInc            = MDMA_SRC_INC_DISABLE;
    MDMA_Handle.Init.DestinationInc       = MDMA_DEST_INC_DISABLE;
    MDMA_Handle.Init.SourceDataSize       = MDMA_SRC_DATASIZE_WORD;
    MDMA_Handle.Init.DestDataSize         = MDMA_DEST_DATASIZE_WORD;
    MDMA_Handle.Init.DataAlignment        = MDMA_DATAALIGN_PACKENABLE;
    MDMA_Handle.Init.SourceBurst          = MDMA_SOURCE_BURST_SINGLE;
    MDMA_Handle.Init.DestBurst            = MDMA_DEST_BURST_SINGLE;
    MDMA_Handle.Init.BufferTransferLength = 4;
      
    MDMA_Handle.Init.SourceBlockAddressOffset  = 0;
    MDMA_Handle.Init.DestBlockAddressOffset    = 0;

    if (HAL_MDMA_Init(&MDMA_Handle) != HAL_OK)
    {
        return -1;
    }

    /*S3:创建各个节点*/
    mdmaLinkNodeConfig.Init.TransferTriggerMode  = MDMA_REPEAT_BLOCK_TRANSFER;
    mdmaLinkNodeConfig.Init.Priority             = MDMA_PRIORITY_HIGH;
    mdmaLinkNodeConfig.Init.Endianness           = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    mdmaLinkNodeConfig.Init.SourceInc            = MDMA_SRC_INC_DISABLE;
    mdmaLinkNodeConfig.Init.DestinationInc       = MDMA_DEST_INC_WORD;
    mdmaLinkNodeConfig.Init.SourceDataSize       = MDMA_SRC_DATASIZE_WORD;
    mdmaLinkNodeConfig.Init.DestDataSize         = MDMA_DEST_DATASIZE_WORD;
    mdmaLinkNodeConfig.Init.DataAlignment        = MDMA_DATAALIGN_PACKENABLE;
    mdmaLinkNodeConfig.Init.SourceBurst          = MDMA_SOURCE_BURST_SINGLE;
    mdmaLinkNodeConfig.Init.DestBurst            = MDMA_DEST_BURST_SINGLE;
    
    mdmaLinkNodeConfig.Init.DestBlockAddressOffset = 0;
    mdmaLinkNodeConfig.BlockDataLength             = 4;
    mdmaLinkNodeConfig.BlockCount                  = cfg->pointCnt;
    
    for (u8BlockLoop = 0; u8BlockLoop < cfg->blockCnt*2; u8BlockLoop++)
    {
        for (u8NodeLoop = 0; u8NodeLoop < cfg->chs; u8NodeLoop++)
        {        
            if(u8NodeLoop < cfg->chX)
            {
                mdmaLinkNodeConfig.Init.Request  = MDMA_REQUEST_DMA1_Stream0_TC;
                mdmaLinkNodeConfig.Init.BufferTransferLength = 4 * cfg->chX;
                mdmaLinkNodeConfig.Init.SourceBlockAddressOffset  = 4 * cfg->chX;
                if (u8BlockLoop % 2 == 0) // DMA Ping buf
                {
                    mdmaLinkNodeConfig.SrcAddress = (INT32U)cfg->pPingDMA_A + 4 * u8NodeLoop;
                }
                else // DMA Pong buf
                {
                    mdmaLinkNodeConfig.SrcAddress = (INT32U)cfg->pPongDMA_A + 4 * u8NodeLoop;
                }
            }
            else
            {
                mdmaLinkNodeConfig.Init.Request  = MDMA_REQUEST_DMA1_Stream2_TC;
                mdmaLinkNodeConfig.Init.BufferTransferLength = 4 * cfg->chY;
                mdmaLinkNodeConfig.Init.SourceBlockAddressOffset  = 4 * cfg->chY;
                if (u8BlockLoop % 2 == 0) // DMA Ping buf
                {
                    mdmaLinkNodeConfig.SrcAddress = (INT32U)cfg->pPingDMA_B + 4 * (u8NodeLoop-cfg->chX);
                }
                else // DMA Pong buf
                {
                    mdmaLinkNodeConfig.SrcAddress = (INT32U)cfg->pPongDMA_B + 4 * (u8NodeLoop-cfg->chX);
                }
            }
            if (u8BlockLoop < cfg->blockCnt) // MDMA Ping buf
            {
                mdmaLinkNodeConfig.DstAddress = (INT32U)cfg->pPingMDMA + 4*cfg->pointCnt*u8NodeLoop + 4*cfg->pointCnt*cfg->chs*u8BlockLoop;
            }
            else // MDMA Pong buf
            {
                mdmaLinkNodeConfig.DstAddress = (INT32U)cfg->pPongMDMA + 4*cfg->pointCnt*u8NodeLoop + 4*cfg->pointCnt*cfg->chs*(u8BlockLoop-cfg->blockCnt);
            }
            // 对此次DMA buf处理已完成，清除DMA传输完成标志
            if (u8NodeLoop == (cfg->chX - 1))
            {
                mdmaLinkNodeConfig.PostRequestMaskAddress = (INT32U)(&(DMA1->LIFCR));
                mdmaLinkNodeConfig.PostRequestMaskData = DMA_LIFCR_CTCIF0;
            }
            else if(u8NodeLoop == (cfg->chs -1))
            {
                mdmaLinkNodeConfig.PostRequestMaskAddress = (INT32U)(&(DMA1->LIFCR));
                mdmaLinkNodeConfig.PostRequestMaskData = DMA_LIFCR_CTCIF2;
            }
            else
            {
                mdmaLinkNodeConfig.PostRequestMaskAddress = 0;
                mdmaLinkNodeConfig.PostRequestMaskData = 0;
            }

            u8NodeIndex = u8BlockLoop * cfg->chs + u8NodeLoop;
            HAL_MDMA_LinkedList_CreateNode(&Xfer_Node[u8NodeIndex], &mdmaLinkNodeConfig);
            if (HAL_MDMA_LinkedList_AddNode(&MDMA_Handle, &Xfer_Node[u8NodeIndex], 0) != HAL_OK)
            {
                return FALSE;
            }    
        }
    }    
    
    /* Make the linked list circular */
    HAL_MDMA_LinkedList_EnableCircularMode(&MDMA_Handle);
    
    /*S4:Select Callbacks functions called after Transfer complete and Transfer error*/
    HAL_MDMA_RegisterCallback(&MDMA_Handle, HAL_MDMA_XFER_REPBLOCKCPLT_CB_ID, MDMA_RepBlockCpltCallback);
    HAL_MDMA_RegisterCallback(&MDMA_Handle, HAL_MDMA_XFER_ERROR_CB_ID, MDMA_TransferErrorCallback);

    mdma_callback = cfg->callback;
    
    /*S5:Configure NVIC for MDMA transfer complete/error interrupts*/
    /* Set Interrupt Group Priority */
    HAL_NVIC_SetPriority(MDMA_IRQn, 8, 0);
    /* Enable the MDMA channel global Interrupt */
    HAL_NVIC_EnableIRQ(MDMA_IRQn);
    
    return 0;
}


int dal_mdma_start(void)
{
    if (HAL_MDMA_Start_IT(&MDMA_Handle, 
                           (INT32U)&dummy_source,
                           (INT32U)&dummy_destination, 
                           4, 1) != HAL_OK)
    {
        return -1;
    }
    __HAL_MDMA_DISABLE_IT(&MDMA_Handle, MDMA_IT_CTC);
    __HAL_MDMA_DISABLE_IT(&MDMA_Handle, MDMA_IT_BT);
    __HAL_MDMA_DISABLE_IT(&MDMA_Handle, MDMA_IT_BFTC);
    
    return 0;
}


int dal_mdma_stop(void)
{
    HAL_MDMA_Abort(&MDMA_Handle);
    
    return 0;
}



