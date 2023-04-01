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
    
    /*S1:SAI1_A DMA配置*/
    /*S1-1:配置Vib接收DMA通道*/
    SAI1A_DMA_Handler.Instance = DMA1_Stream0;                                // DMA1数据流0
    SAI1A_DMA_Handler.Init.Request = DMA_REQUEST_SAI1_A;                    // SAI1 Block A
    SAI1A_DMA_Handler.Init.Direction = DMA_PERIPH_TO_MEMORY;                // 外设到存储器模式
    SAI1A_DMA_Handler.Init.PeriphInc = DMA_PINC_DISABLE;                    // 外设非增量模式
    SAI1A_DMA_Handler.Init.MemInc = DMA_MINC_ENABLE;                        // 存储器增量模式
    SAI1A_DMA_Handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;        // 外设数据长度:32位
    SAI1A_DMA_Handler.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;            // 存储器数据长度:32位
    SAI1A_DMA_Handler.Init.Mode = DMA_CIRCULAR;                                // 使用循环模式
    SAI1A_DMA_Handler.Init.Priority = DMA_PRIORITY_HIGH;                       // 优先级高
    SAI1A_DMA_Handler.Init.FIFOMode = DMA_FIFOMODE_DISABLE;                  // 不使用FIFO
    SAI1A_DMA_Handler.Init.MemBurst = DMA_MBURST_SINGLE;                     // 存储器单次突发传输
    SAI1A_DMA_Handler.Init.PeriphBurst = DMA_PBURST_SINGLE;                  // 外设突发单次传输
    
    /*S1-2:初始化DMA*/
    if (HAL_DMA_DeInit(&SAI1A_DMA_Handler) != HAL_OK) {                     // 先清除以前的设置
        return -1;
    }    
    if (HAL_DMA_Init(&SAI1A_DMA_Handler) != HAL_OK) {                        // 初始化DMA
    
        return -1;
    }
    
    /*S1-3:关联ADC句柄和DMA句柄*/
    __HAL_LINKDMA(&SAI1A_Handler, hdmarx, SAI1A_DMA_Handler);                // 将DMA与SAI1A联系起来
    
    /*S2:SAI1_B DMA配置*/
    /*S2-1:配置Vib接收DMA通道*/
    SAI1B_DMA_Handler.Instance = DMA1_Stream2;                                // DMA1数据流2
    SAI1B_DMA_Handler.Init.Request = DMA_REQUEST_SAI1_B;                    // SAI1 Block B
    SAI1B_DMA_Handler.Init.Direction = DMA_PERIPH_TO_MEMORY;                // 外设到存储器模式
    SAI1B_DMA_Handler.Init.PeriphInc = DMA_PINC_DISABLE;                    // 外设非增量模式
    SAI1B_DMA_Handler.Init.MemInc = DMA_MINC_ENABLE;                        // 存储器增量模式
    SAI1B_DMA_Handler.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;        // 外设数据长度:32位
    SAI1B_DMA_Handler.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;            // 存储器数据长度:32位
    SAI1B_DMA_Handler.Init.Mode = DMA_CIRCULAR;                                // 使用循环模式
    SAI1B_DMA_Handler.Init.Priority = DMA_PRIORITY_HIGH;                       // 优先级高
    SAI1B_DMA_Handler.Init.FIFOMode = DMA_FIFOMODE_DISABLE;                  // 不使用FIFO
    SAI1B_DMA_Handler.Init.MemBurst = DMA_MBURST_SINGLE;                     // 存储器单次突发传输
    SAI1B_DMA_Handler.Init.PeriphBurst = DMA_PBURST_SINGLE;                  // 外设突发单次传输
    
    /*S2-2:初始化DMA*/
    if (HAL_DMA_DeInit(&SAI1B_DMA_Handler) != HAL_OK)                         // 先清除以前的设置
    {
        return -1;
    }    
    if (HAL_DMA_Init(&SAI1B_DMA_Handler) != HAL_OK)                         // 初始化DMA
    {
        return -1;
    }
    
    /*S2-3:关联ADC句柄和DMA句柄*/
    __HAL_LINKDMA(&SAI1B_Handler, hdmarx, SAI1B_DMA_Handler);                // 将DMA与SAI1A联系起来
    
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

    if (HAL_SAI_DeInit(&SAI1A_Handler) != HAL_OK)                           // 清除以前的配置
    {
        return -1;
    }
    SAI1A_Handler.Instance = SAI1_Block_A;                                    // SAI1 Block A
    SAI1A_Handler.Init.AudioMode = SAI_MODESLAVE_RX;                          // 从接收器
    SAI1A_Handler.Init.Synchro = SAI_SYNCHRONOUS_EXT_SAI4;                     // SAI1-A与SAI4同步
    SAI1A_Handler.Init.SynchroExt = SAI_SYNCEXT_DISABLE;                    // 无同步输出信号
    SAI1A_Handler.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;                  // 当SAIEN置1时驱动音频模块输出
    SAI1A_Handler.Init.NoDivider = SAI_MASTERDIVIDER_DISABLE;                  // 禁止主时钟分频器
    SAI1A_Handler.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;                 // 阈值设置为半满
    SAI1A_Handler.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV;           // 自定义采样频率
    SAI1A_Handler.Init.Mckdiv = 2;                                            // 2分频(13.1072MHz)
    SAI1A_Handler.Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;        // 256 x Fs
    SAI1A_Handler.Init.MonoStereoMode = SAI_STEREOMODE;                       // 立体声模式
    SAI1A_Handler.Init.Protocol = SAI_FREE_PROTOCOL;                          // 设置SAI1协议为：自由协议(支持TDM)
    SAI1A_Handler.Init.DataSize = SAI_DATASIZE_24;                            // 设置数据大小：24位ADC
    SAI1A_Handler.Init.FirstBit = SAI_FIRSTBIT_MSB;                            // 数据MSB位优先
    SAI1A_Handler.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;        // 数据在下降沿输出上升沿采样

    // 帧设置
    SAI1A_Handler.FrameInit.FrameLength = 256;                               // 一个帧中有256个位时钟周期
    SAI1A_Handler.FrameInit.ActiveFrameLength = 1;                             // 帧同步有效电平长度，持续一个位时钟周期
    SAI1A_Handler.FrameInit.FSDefinition = SAI_FS_STARTFRAME;                // FS信号为起始帧信号
    SAI1A_Handler.FrameInit.FSPolarity = SAI_FS_ACTIVE_HIGH;                   // FS高电平有效
    SAI1A_Handler.FrameInit.FSOffset = SAI_FS_FIRSTBIT;                        // 在Slot0的第一位上使能FS

    // Slot设置
    SAI1A_Handler.SlotInit.FirstBitOffset = 0;                                // Slot偏移(FBOFF)为0
    SAI1A_Handler.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;                   // Slot大小与数据大小一致(24位)
    SAI1A_Handler.SlotInit.SlotNumber = 8;                                    // 8个Slot    
    SAI1A_Handler.SlotInit.SlotActive = (INT8U)u16ChnBits;                    // 使能采集通道对应Slot
    
    if (HAL_SAI_Init(&SAI1A_Handler) != HAL_OK)                               // 初始化SAI1A
    {
        return -1;
    }
    return 0;
}

static int SAI1B_Cfg(U16 u16ChnBits)
{

    if (HAL_SAI_DeInit(&SAI1B_Handler) != HAL_OK)                           // 清除以前的配置
    {
        return -1;
    }
    SAI1B_Handler.Instance = SAI1_Block_B;                                    // SAI1 Block B
    SAI1B_Handler.Init.AudioMode = SAI_MODESLAVE_RX;                          // 从接收器
    SAI1B_Handler.Init.Synchro = SAI_SYNCHRONOUS_EXT_SAI4;                     // SAI1-B与SAI4同步
    SAI1B_Handler.Init.SynchroExt = SAI_SYNCEXT_DISABLE;                    // 无同步输出信号
    SAI1B_Handler.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;                  // 当SAIEN置1时驱动音频模块输出
    SAI1B_Handler.Init.NoDivider = SAI_MASTERDIVIDER_DISABLE;                  // 禁止主时钟分频器
    SAI1B_Handler.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;                 // 阈值设置为半满
    SAI1B_Handler.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV;           // 自定义采样频率
    SAI1B_Handler.Init.Mckdiv = 2;                                            // 2分频(13.1072MHz)
    SAI1B_Handler.Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;        // 256 x Fs
    SAI1B_Handler.Init.MonoStereoMode = SAI_STEREOMODE;                       // 立体声模式
    SAI1B_Handler.Init.Protocol = SAI_FREE_PROTOCOL;                          // 设置SAI1协议为：自由协议(支持TDM)
    SAI1B_Handler.Init.DataSize = SAI_DATASIZE_24;                            // 设置数据大小：24位ADC
    SAI1B_Handler.Init.FirstBit = SAI_FIRSTBIT_MSB;                            // 数据MSB位优先
    SAI1B_Handler.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;        // 数据在下降沿输出上升沿采样

    // 帧设置
    SAI1B_Handler.FrameInit.FrameLength = 256;                               // 一个帧中有256个位时钟周期
    SAI1B_Handler.FrameInit.ActiveFrameLength = 1;                             // 帧同步有效电平长度，持续一个位时钟周期
    SAI1B_Handler.FrameInit.FSDefinition = SAI_FS_STARTFRAME;                // FS信号为起始帧信号
    SAI1B_Handler.FrameInit.FSPolarity = SAI_FS_ACTIVE_HIGH;                   // FS高电平有效
    SAI1B_Handler.FrameInit.FSOffset = SAI_FS_FIRSTBIT;                        // 在Slot0的第一位上使能FS

    // Slot设置
    SAI1B_Handler.SlotInit.FirstBitOffset = 0;                                // Slot偏移(FBOFF)为0
    SAI1B_Handler.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;                   // Slot大小与数据大小一致(24位)
    SAI1B_Handler.SlotInit.SlotNumber = 8;                                    // 8个Slot    
    SAI1B_Handler.SlotInit.SlotActive = (INT8U)(u16ChnBits >> 8);            // 使能采集通道对应Slot
    
    if (HAL_SAI_Init(&SAI1B_Handler) != HAL_OK)                               // 初始化SAI1B
    {
        return -1;
    }
    return 0;
}


static int sai_clk_start(void)
{
    SAI_HandleTypeDef SAI4A_Handler = {0};
    if (HAL_SAI_DeInit(&SAI4A_Handler) != HAL_OK) {                           // 清除以前的配置
        return -1;
    }
    
    SAI4A_Handler.Instance = SAI4_Block_A;                                    // SAI4 Block A
    SAI4A_Handler.Init.AudioMode = SAI_MODEMASTER_RX;                         // 主接收器
    SAI4A_Handler.Init.Synchro = SAI_ASYNCHRONOUS;                            // 音频模块异步
    SAI4A_Handler.Init.SynchroExt = SAI_SYNCEXT_OUTBLOCKA_ENABLE;             // SAI4-A用于其它SAI的进一步同步
    SAI4A_Handler.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;                  // 该位置1后立即驱动音频模块输出
    SAI4A_Handler.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;                  // 使能主时钟分频器
    SAI4A_Handler.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_HF;                  // 阈值设置为半满
    SAI4A_Handler.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV;           // 自定义采样频率
    SAI4A_Handler.Init.Mckdiv = 2;                                            // 2分频(13.1072MHz)
    SAI4A_Handler.Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;        // 256 x Fs
    SAI4A_Handler.Init.MonoStereoMode = SAI_STEREOMODE;                       // 立体声模式
    SAI4A_Handler.Init.Protocol = SAI_FREE_PROTOCOL;                          // 设置SAI4协议为：自由协议(支持TDM)
    SAI4A_Handler.Init.DataSize = SAI_DATASIZE_24;                            // 设置数据大小：24位ADC
    SAI4A_Handler.Init.FirstBit = SAI_FIRSTBIT_MSB;                           // 数据MSB位优先
    SAI4A_Handler.Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;          // 数据在下降沿输出上升沿采样

    // 帧设置
    SAI4A_Handler.FrameInit.FrameLength = 256;                                // 一个帧中有256个位时钟周期
    SAI4A_Handler.FrameInit.ActiveFrameLength = 1;                            // 帧同步有效电平长度，持续一个位时钟周期
    SAI4A_Handler.FrameInit.FSDefinition = SAI_FS_STARTFRAME;                 // FS信号为起始帧信号
    SAI4A_Handler.FrameInit.FSPolarity = SAI_FS_ACTIVE_HIGH;                  // FS高电平有效
    SAI4A_Handler.FrameInit.FSOffset = SAI_FS_FIRSTBIT;                       // 在Slot0的第一位上使能FS

    // Slot设置
    SAI4A_Handler.SlotInit.FirstBitOffset = 0;                                // Slot偏移(FBOFF)为0
    SAI4A_Handler.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;                  // Slot大小与数据大小一致(24位)
    SAI4A_Handler.SlotInit.SlotNumber = 8;                                    // 8个Slot
    SAI4A_Handler.SlotInit.SlotActive = 0;                                    // 所有Slot无效
    
    if (HAL_SAI_Init(&SAI4A_Handler) != HAL_OK) {                             // 初始化SAI4A
        return -1;
    }
    
    __HAL_SAI_ENABLE(&SAI4A_Handler);       // 使能SAI4A
    
    return 0;
}
static void SAI1A_DMA_Enable(U8 onoff)
{
    U32 tempreg = 0;
    tempreg = SAI1_Block_A->CR1;            // 先读出以前的设置
    if(onoff) {
        tempreg |= 1<<17;                   // 使能DMA
    }
    else {
        tempreg &= ~(1<<17);
    }
    SAI1_Block_A->CR1 = tempreg;            // 写入CR1寄存器中
}
static void SAI1B_DMA_Enable(U8 onoff)
{
    U32 tempreg = 0;
    tempreg = SAI1_Block_B->CR1;            // 先读出以前的设置            
    if(onoff) {
        tempreg |= 1<<17;                   // 使能DMA
    }
    else {
        tempreg &= ~(1<<17);
    }
    SAI1_Block_B->CR1 = tempreg;            // 写入CR1寄存器中
}

////////////////////////////////////////////////////////////



int sai_init(void)
{    
    /*S1:使能外设时钟*/
    __HAL_RCC_SAI1_CLK_ENABLE();                // 使能SAI1时钟
    __HAL_RCC_SAI4_CLK_ENABLE();                // 使能SAI4时钟
    __HAL_RCC_GPIOE_CLK_ENABLE();               // 使能GPIOE时钟
    
    /*S4:生成SAI时钟信号*/
    if (sai_clk_start()) {
        return -1;
    }
    
    dma_init();
    mdma_init();
    
    return 0;
}


/*
描述：SAI接口配置函数
Note：u8ChnBits 采集通道位图(每个SAI子模块对应8路振动通道,SAI1A对应前8路振动，SAI1B对应后8路振动)
*/
int sai_config(sai_cfg_t *cfg)
{
    GPIO_InitTypeDef init={0};
    
    if(cfg->e3Mux) {
        init.Pin =  GPIO_PIN_6;                  //PE3用于馈电
    }
    else {
        init.Pin =  GPIO_PIN_3  |  GPIO_PIN_6;  // PE3用于扩展板
    }
    
    init.Mode = GPIO_MODE_AF_PP;                // 推挽复用
    init.Pull = GPIO_NOPULL;                    // 没有上/下拉
    init.Speed = GPIO_SPEED_FREQ_MEDIUM;        // 中速
    init.Alternate = GPIO_AF6_SAI1;             // 复用为SAI1   
    HAL_GPIO_Init(GPIOE, &init);
    
    init.Pin = GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5;
    init.Alternate = GPIO_AF8_SAI4;             // 复用为SAI4
    HAL_GPIO_Init(GPIOE, &init);
    
    if(SAI1A_Cfg(cfg->chBits)) {
        return -1;
    }
    if(SAI1B_Cfg(cfg->chBits)) {
        return -1;
    }
    HAL_NVIC_SetPriority(SAI1_IRQn, INT_PRI_SAI, 0);
    HAL_NVIC_EnableIRQ(SAI1_IRQn);
    
    __HAL_SAI_ENABLE_IT(&SAI1A_Handler, SAI_IT_OVRUDR); // 使能SAI1A溢出中断
    __HAL_SAI_ENABLE_IT(&SAI1B_Handler, SAI_IT_OVRUDR); // 使能SAI1B溢出中断
    
    
    mdma_config(&cfg->mdma);
    mSaiCfg = *cfg;
    
    return 0;
}


int sai_start(void)
{
    sai_cfg_t *cfg=&mSaiCfg;
    
    SAI1A_DMA_Enable(1);                    // 使能SAIA的DMA功能
    __HAL_SAI_ENABLE(&SAI1A_Handler);       // 使能SAI1A
    
    SAI1B_DMA_Enable(1);                    // 使能SAIB的DMA功能
    __HAL_SAI_ENABLE(&SAI1B_Handler);       // 使能SAI1B
    
    dma_start(cfg->mdma.pPingDMA_A, cfg->mdma.pPongDMA_A, cfg->mdma.pPingDMA_B, cfg->mdma.pPongDMA_B, cfg->mdma.pointCnt, cfg->mdma.chX, cfg->mdma.chY);
    mdma_start();
    
    return 0;
}


int sai_stop(void)
{
    HAL_NVIC_DisableIRQ(SAI1_IRQn);
    __HAL_SAI_DISABLE(&SAI1A_Handler);      // 关闭SAI1A    
    SAI1A_DMA_Enable(0);                    // 关闭SAI1A的DMA功能
    __HAL_SAI_DISABLE(&SAI1B_Handler);      // 关闭SAI1B
    SAI1B_DMA_Enable(0);                    // 关闭SAI1B的DMA功能
    
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

