#include "dal/sai.h"
#include "dal/mdma.h"
#include "log.h"
#include "cfg.h"


SAI_HandleTypeDef    SAI1A_Handler = {0};
SAI_HandleTypeDef    SAI1B_Handler = {0};
DMA_HandleTypeDef    SAI1A_DMA_Handler = {0};
DMA_HandleTypeDef    SAI1B_DMA_Handler = {0};

static sai_cfg_t mSaiCfg;
static void SAI1A_DMA_Enable(U8 onoff);
static void SAI1B_DMA_Enable(U8 onoff);
static int sai_clk_start(void);



static int dma_init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_MDMA_CLK_ENABLE();
    
    /*S1:SAI1_A DMA����*/
    /*S1-1:����Vib����DMAͨ��*/
    SAI1A_DMA_Handler.Instance = DMA1_Stream0;                                // DMA1������0
    SAI1A_DMA_Handler.Init.Request = DMA_REQUEST_SAI1_A;                    // SAI1 Block A
    SAI1A_DMA_Handler.Init.Direction = DMA_PERIPH_TO_MEMORY;                // ���赽�洢��ģʽ
    SAI1A_DMA_Handler.Init.PeriphInc = DMA_PINC_DISABLE;                    // ���������ģʽ
    SAI1A_DMA_Handler.Init.MemInc = DMA_MINC_ENABLE;                        // �洢������ģʽ
    SAI1A_DMA_Handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;        // �������ݳ���:32λ
    SAI1A_DMA_Handler.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;            // �洢�����ݳ���:32λ
    SAI1A_DMA_Handler.Init.Mode = DMA_CIRCULAR;                                // ʹ��ѭ��ģʽ
    SAI1A_DMA_Handler.Init.Priority = DMA_PRIORITY_HIGH;                       // ���ȼ���
    SAI1A_DMA_Handler.Init.FIFOMode = DMA_FIFOMODE_DISABLE;                  // ��ʹ��FIFO
    SAI1A_DMA_Handler.Init.MemBurst = DMA_MBURST_SINGLE;                     // �洢������ͻ������
    SAI1A_DMA_Handler.Init.PeriphBurst = DMA_PBURST_SINGLE;                  // ����ͻ�����δ���
    
    /*S1-2:��ʼ��DMA*/
    if (HAL_DMA_DeInit(&SAI1A_DMA_Handler) != HAL_OK) {                     // �������ǰ������
        return -1;
    }    
    if (HAL_DMA_Init(&SAI1A_DMA_Handler) != HAL_OK) {                        // ��ʼ��DMA
    
        return -1;
    }
    
    /*S1-3:����ADC�����DMA���*/
    __HAL_LINKDMA(&SAI1A_Handler, hdmarx, SAI1A_DMA_Handler);                // ��DMA��SAI1A��ϵ����
    
    /*S2:SAI1_B DMA����*/
    /*S2-1:����Vib����DMAͨ��*/
    SAI1B_DMA_Handler.Instance = DMA1_Stream2;                                // DMA1������2
    SAI1B_DMA_Handler.Init.Request = DMA_REQUEST_SAI1_B;                    // SAI1 Block B
    SAI1B_DMA_Handler.Init.Direction = DMA_PERIPH_TO_MEMORY;                // ���赽�洢��ģʽ
    SAI1B_DMA_Handler.Init.PeriphInc = DMA_PINC_DISABLE;                    // ���������ģʽ
    SAI1B_DMA_Handler.Init.MemInc = DMA_MINC_ENABLE;                        // �洢������ģʽ
    SAI1B_DMA_Handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;        // �������ݳ���:32λ
    SAI1B_DMA_Handler.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;            // �洢�����ݳ���:32λ
    SAI1B_DMA_Handler.Init.Mode = DMA_CIRCULAR;                                // ʹ��ѭ��ģʽ
    SAI1B_DMA_Handler.Init.Priority = DMA_PRIORITY_HIGH;                       // ���ȼ���
    SAI1B_DMA_Handler.Init.FIFOMode = DMA_FIFOMODE_DISABLE;                  // ��ʹ��FIFO
    SAI1B_DMA_Handler.Init.MemBurst = DMA_MBURST_SINGLE;                     // �洢������ͻ������
    SAI1B_DMA_Handler.Init.PeriphBurst = DMA_PBURST_SINGLE;                  // ����ͻ�����δ���
    
    /*S2-2:��ʼ��DMA*/
    if (HAL_DMA_DeInit(&SAI1B_DMA_Handler) != HAL_OK)                         // �������ǰ������
    {
        return -1;
    }    
    if (HAL_DMA_Init(&SAI1B_DMA_Handler) != HAL_OK)                         // ��ʼ��DMA
    {
        return -1;
    }
    
    /*S2-3:����ADC�����DMA���*/
    __HAL_LINKDMA(&SAI1B_Handler, hdmarx, SAI1B_DMA_Handler);                // ��DMA��SAI1A��ϵ����
    
    return 0;
}

static int dma_start( 
                U32* pBufPingDstDMA_A, 
                U32* pBufPongDstDMA_A,
                U32* pBufPingDstDMA_B, 
                U32* pBufPongDstDMA_B,
                U16  BufPointNum, 
                U16  u16BeforeVibChannelCnt,
                U16  u16AfterVibChannelCnt)
{
    int DataLength = 0;
    
    DataLength = u16BeforeVibChannelCnt * BufPointNum;
    if (HAL_DMAEx_MultiBufferStart(&SAI1A_DMA_Handler, (U32)&SAI1_Block_A->DR,
                                (U32)pBufPingDstDMA_A, (U32)pBufPongDstDMA_A, DataLength) != HAL_OK)
    {
        return -1;     
    }
    DataLength = u16AfterVibChannelCnt * BufPointNum;
    if (HAL_DMAEx_MultiBufferStart(&SAI1B_DMA_Handler, (U32)&SAI1_Block_B->DR,
                                (U32)pBufPingDstDMA_B, (U32)pBufPongDstDMA_B, DataLength) != HAL_OK)
    {
        return -1;     
    }
    
    return 0;
}
static int dma_stop(void)
{
    HAL_DMA_Abort(&SAI1A_DMA_Handler);
    HAL_DMA_Abort(&SAI1B_DMA_Handler);
    return 0;
}


static int SAI1A_Cfg(U16 u16ChnBits)
{

    if (HAL_SAI_DeInit(&SAI1A_Handler) != HAL_OK)                           // �����ǰ������
    {
        return -1;
    }
    SAI1A_Handler.Instance = SAI1_Block_A;                                    // SAI1 Block A
    SAI1A_Handler.Init.AudioMode = SAI_MODESLAVE_RX;                          // �ӽ�����
    SAI1A_Handler.Init.Synchro = SAI_SYNCHRONOUS_EXT_SAI4;                     // SAI1-A��SAI4ͬ��
    SAI1A_Handler.Init.SynchroExt = SAI_SYNCEXT_DISABLE;                    // ��ͬ������ź�
    SAI1A_Handler.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;                  // ��SAIEN��1ʱ������Ƶģ�����
    SAI1A_Handler.Init.NoDivider = SAI_MASTERDIVIDER_DISABLE;                  // ��ֹ��ʱ�ӷ�Ƶ��
    SAI1A_Handler.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;                 // ��ֵ����Ϊ����
    SAI1A_Handler.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV;           // �Զ������Ƶ��
    SAI1A_Handler.Init.Mckdiv = 2;                                            // 2��Ƶ(13.1072MHz)
    SAI1A_Handler.Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;        // 256 x Fs
    SAI1A_Handler.Init.MonoStereoMode = SAI_STEREOMODE;                       // ������ģʽ
    SAI1A_Handler.Init.Protocol = SAI_FREE_PROTOCOL;                          // ����SAI1Э��Ϊ������Э��(֧��TDM)
    SAI1A_Handler.Init.DataSize = SAI_DATASIZE_24;                            // �������ݴ�С��24λADC
    SAI1A_Handler.Init.FirstBit = SAI_FIRSTBIT_MSB;                            // ����MSBλ����
    SAI1A_Handler.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;        // �������½�����������ز���

    // ֡����
    SAI1A_Handler.FrameInit.FrameLength = 256;                               // һ��֡����256��λʱ������
    SAI1A_Handler.FrameInit.ActiveFrameLength = 1;                             // ֡ͬ����Ч��ƽ���ȣ�����һ��λʱ������
    SAI1A_Handler.FrameInit.FSDefinition = SAI_FS_STARTFRAME;                // FS�ź�Ϊ��ʼ֡�ź�
    SAI1A_Handler.FrameInit.FSPolarity = SAI_FS_ACTIVE_HIGH;                   // FS�ߵ�ƽ��Ч
    SAI1A_Handler.FrameInit.FSOffset = SAI_FS_FIRSTBIT;                        // ��Slot0�ĵ�һλ��ʹ��FS

    // Slot����
    SAI1A_Handler.SlotInit.FirstBitOffset = 0;                                // Slotƫ��(FBOFF)Ϊ0
    SAI1A_Handler.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;                   // Slot��С�����ݴ�Сһ��(24λ)
    SAI1A_Handler.SlotInit.SlotNumber = 8;                                    // 8��Slot    
    SAI1A_Handler.SlotInit.SlotActive = (INT8U)u16ChnBits;                    // ʹ�ܲɼ�ͨ����ӦSlot
    
    if (HAL_SAI_Init(&SAI1A_Handler) != HAL_OK)                               // ��ʼ��SAI1A
    {
        return -1;
    }
    return 0;
}

static int SAI1B_Cfg(U16 u16ChnBits)
{

    if (HAL_SAI_DeInit(&SAI1B_Handler) != HAL_OK)                           // �����ǰ������
    {
        return -1;
    }
    SAI1B_Handler.Instance = SAI1_Block_B;                                    // SAI1 Block B
    SAI1B_Handler.Init.AudioMode = SAI_MODESLAVE_RX;                          // �ӽ�����
    SAI1B_Handler.Init.Synchro = SAI_SYNCHRONOUS_EXT_SAI4;                     // SAI1-B��SAI4ͬ��
    SAI1B_Handler.Init.SynchroExt = SAI_SYNCEXT_DISABLE;                    // ��ͬ������ź�
    SAI1B_Handler.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;                  // ��SAIEN��1ʱ������Ƶģ�����
    SAI1B_Handler.Init.NoDivider = SAI_MASTERDIVIDER_DISABLE;                  // ��ֹ��ʱ�ӷ�Ƶ��
    SAI1B_Handler.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;                 // ��ֵ����Ϊ����
    SAI1B_Handler.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV;           // �Զ������Ƶ��
    SAI1B_Handler.Init.Mckdiv = 2;                                            // 2��Ƶ(13.1072MHz)
    SAI1B_Handler.Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;        // 256 x Fs
    SAI1B_Handler.Init.MonoStereoMode = SAI_STEREOMODE;                       // ������ģʽ
    SAI1B_Handler.Init.Protocol = SAI_FREE_PROTOCOL;                          // ����SAI1Э��Ϊ������Э��(֧��TDM)
    SAI1B_Handler.Init.DataSize = SAI_DATASIZE_24;                            // �������ݴ�С��24λADC
    SAI1B_Handler.Init.FirstBit = SAI_FIRSTBIT_MSB;                            // ����MSBλ����
    SAI1B_Handler.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;        // �������½�����������ز���

    // ֡����
    SAI1B_Handler.FrameInit.FrameLength = 256;                               // һ��֡����256��λʱ������
    SAI1B_Handler.FrameInit.ActiveFrameLength = 1;                             // ֡ͬ����Ч��ƽ���ȣ�����һ��λʱ������
    SAI1B_Handler.FrameInit.FSDefinition = SAI_FS_STARTFRAME;                // FS�ź�Ϊ��ʼ֡�ź�
    SAI1B_Handler.FrameInit.FSPolarity = SAI_FS_ACTIVE_HIGH;                   // FS�ߵ�ƽ��Ч
    SAI1B_Handler.FrameInit.FSOffset = SAI_FS_FIRSTBIT;                        // ��Slot0�ĵ�һλ��ʹ��FS

    // Slot����
    SAI1B_Handler.SlotInit.FirstBitOffset = 0;                                // Slotƫ��(FBOFF)Ϊ0
    SAI1B_Handler.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;                   // Slot��С�����ݴ�Сһ��(24λ)
    SAI1B_Handler.SlotInit.SlotNumber = 8;                                    // 8��Slot    
    SAI1B_Handler.SlotInit.SlotActive = (INT8U)(u16ChnBits >> 8);            // ʹ�ܲɼ�ͨ����ӦSlot
    
    if (HAL_SAI_Init(&SAI1B_Handler) != HAL_OK)                               // ��ʼ��SAI1B
    {
        return -1;
    }
    return 0;
}


static int sai_clk_start(void)
{
    SAI_HandleTypeDef SAI4A_Handler = {0};
    if (HAL_SAI_DeInit(&SAI4A_Handler) != HAL_OK) {                           // �����ǰ������
        return -1;
    }
    
    SAI4A_Handler.Instance = SAI4_Block_A;                                    // SAI4 Block A
    SAI4A_Handler.Init.AudioMode = SAI_MODEMASTER_RX;                         // ��������
    SAI4A_Handler.Init.Synchro = SAI_ASYNCHRONOUS;                            // ��Ƶģ���첽
    SAI4A_Handler.Init.SynchroExt = SAI_SYNCEXT_OUTBLOCKA_ENABLE;             // SAI4-A��������SAI�Ľ�һ��ͬ��
    SAI4A_Handler.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;                  // ��λ��1������������Ƶģ�����
    SAI4A_Handler.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;                  // ʹ����ʱ�ӷ�Ƶ��
    SAI4A_Handler.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;                  // ��ֵ����Ϊ����
    SAI4A_Handler.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV;           // �Զ������Ƶ��
    SAI4A_Handler.Init.Mckdiv = 2;                                            // 2��Ƶ(13.1072MHz)
    SAI4A_Handler.Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;        // 256 x Fs
    SAI4A_Handler.Init.MonoStereoMode = SAI_STEREOMODE;                       // ������ģʽ
    SAI4A_Handler.Init.Protocol = SAI_FREE_PROTOCOL;                          // ����SAI4Э��Ϊ������Э��(֧��TDM)
    SAI4A_Handler.Init.DataSize = SAI_DATASIZE_24;                            // �������ݴ�С��24λADC
    SAI4A_Handler.Init.FirstBit = SAI_FIRSTBIT_MSB;                           // ����MSBλ����
    SAI4A_Handler.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;          // �������½�����������ز���

    // ֡����
    SAI4A_Handler.FrameInit.FrameLength = 256;                                // һ��֡����256��λʱ������
    SAI4A_Handler.FrameInit.ActiveFrameLength = 1;                            // ֡ͬ����Ч��ƽ���ȣ�����һ��λʱ������
    SAI4A_Handler.FrameInit.FSDefinition = SAI_FS_STARTFRAME;                 // FS�ź�Ϊ��ʼ֡�ź�
    SAI4A_Handler.FrameInit.FSPolarity = SAI_FS_ACTIVE_HIGH;                  // FS�ߵ�ƽ��Ч
    SAI4A_Handler.FrameInit.FSOffset = SAI_FS_FIRSTBIT;                       // ��Slot0�ĵ�һλ��ʹ��FS

    // Slot����
    SAI4A_Handler.SlotInit.FirstBitOffset = 0;                                // Slotƫ��(FBOFF)Ϊ0
    SAI4A_Handler.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;                  // Slot��С�����ݴ�Сһ��(24λ)
    SAI4A_Handler.SlotInit.SlotNumber = 8;                                    // 8��Slot
    SAI4A_Handler.SlotInit.SlotActive = 0;                                    // ����Slot��Ч
    
    if (HAL_SAI_Init(&SAI4A_Handler) != HAL_OK) {                             // ��ʼ��SAI4A
        return -1;
    }
    
    __HAL_SAI_ENABLE(&SAI4A_Handler);       // ʹ��SAI4A
    
    return 0;
}
static void SAI1A_DMA_Enable(U8 onoff)
{
    U32 tempreg = 0;
    tempreg = SAI1_Block_A->CR1;            // �ȶ�����ǰ������
    if(onoff) {
        tempreg |= 1<<17;                   // ʹ��DMA
    }
    else {
        tempreg &= ~(1<<17);
    }
    SAI1_Block_A->CR1 = tempreg;            // д��CR1�Ĵ�����
}
static void SAI1B_DMA_Enable(U8 onoff)
{
    U32 tempreg = 0;
    tempreg = SAI1_Block_B->CR1;            // �ȶ�����ǰ������            
    if(onoff) {
        tempreg |= 1<<17;                   // ʹ��DMA
    }
    else {
        tempreg &= ~(1<<17);
    }
    SAI1_Block_B->CR1 = tempreg;            // д��CR1�Ĵ�����
}

////////////////////////////////////////////////////////////



int sai_init(void)
{    
    /*S1:ʹ������ʱ��*/
    __HAL_RCC_SAI1_CLK_ENABLE();                // ʹ��SAI1ʱ��
    __HAL_RCC_SAI4_CLK_ENABLE();                // ʹ��SAI4ʱ��
    __HAL_RCC_GPIOE_CLK_ENABLE();               // ʹ��GPIOEʱ��
    
    /*S4:����SAIʱ���ź�*/
    if (sai_clk_start()) {
        return -1;
    }
    
    dma_init();
    mdma_init();
    
    return 0;
}


/*
������SAI�ӿ����ú���
Note��u8ChnBits �ɼ�ͨ��λͼ(ÿ��SAI��ģ���Ӧ8·��ͨ��,SAI1A��Ӧǰ8·�񶯣�SAI1B��Ӧ��8·��)
*/
int sai_config(sai_cfg_t *cfg)
{
    GPIO_InitTypeDef init={0};
    
    if(cfg->e3Mux) {
        init.Pin =  GPIO_PIN_6;                  //PE3��������
    }
    else {
        init.Pin =  GPIO_PIN_3  |  GPIO_PIN_6;  // PE3������չ��
    }
    
    init.Mode = GPIO_MODE_AF_PP;                // ���츴��
    init.Pull = GPIO_NOPULL;                    // û����/����
    init.Speed = GPIO_SPEED_FREQ_MEDIUM;        // ����
    init.Alternate = GPIO_AF6_SAI1;             // ����ΪSAI1   
    HAL_GPIO_Init(GPIOE, &init);
    
    init.Pin = GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5;
    init.Alternate = GPIO_AF8_SAI4;             // ����ΪSAI4
    HAL_GPIO_Init(GPIOE, &init);
    
    if(SAI1A_Cfg(cfg->chBits)) {
        return -1;
    }
    if(SAI1B_Cfg(cfg->chBits)) {
        return -1;
    }
    HAL_NVIC_SetPriority(SAI1_IRQn, INT_PRI_SAI, 0);
    HAL_NVIC_EnableIRQ(SAI1_IRQn);
    
    __HAL_SAI_ENABLE_IT(&SAI1A_Handler, SAI_IT_OVRUDR); // ʹ��SAI1A����ж�
    __HAL_SAI_ENABLE_IT(&SAI1B_Handler, SAI_IT_OVRUDR); // ʹ��SAI1B����ж�
    
    
    mdma_config(&cfg->mdma);
    mSaiCfg = *cfg;
    
    return 0;
}


int sai_start(void)
{
    sai_cfg_t *cfg=&mSaiCfg;
    
    SAI1A_DMA_Enable(1);                    // ʹ��SAIA��DMA����
    __HAL_SAI_ENABLE(&SAI1A_Handler);       // ʹ��SAI1A
    
    SAI1B_DMA_Enable(1);                    // ʹ��SAIB��DMA����
    __HAL_SAI_ENABLE(&SAI1B_Handler);       // ʹ��SAI1B
    
    dma_start(cfg->mdma.pPingDMA_A, cfg->mdma.pPongDMA_A, cfg->mdma.pPingDMA_B, cfg->mdma.pPongDMA_B, cfg->mdma.pointCnt, cfg->mdma.chX, cfg->mdma.chY);
    mdma_start();
    
    return 0;
}


int sai_stop(void)
{
    HAL_NVIC_DisableIRQ(SAI1_IRQn);
    __HAL_SAI_DISABLE(&SAI1A_Handler);      // �ر�SAI1A    
    SAI1A_DMA_Enable(0);                    // �ر�SAI1A��DMA����
    __HAL_SAI_DISABLE(&SAI1B_Handler);      // �ر�SAI1B
    SAI1B_DMA_Enable(0);                    // �ر�SAI1B��DMA����
    
    dma_stop();
    mdma_stop();
    
    return 0;
}


void SAI1_IRQHandler(void)
{
    HAL_SAI_IRQHandler(&SAI1A_Handler);
}
void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    LOGE("********** SAI OVRUDR!!!!! **********\n");
}

