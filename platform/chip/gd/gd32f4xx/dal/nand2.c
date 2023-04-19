#include "platform.h"
#include "log.h"
#include "dal/nand.h"
#include "dal/delay.h"


//https://blog.csdn.net/sudaroot/article/details/99640553


typedef volatile uint8_t  vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;


static nand_attr_t nand_attr;
static NAND_HandleTypeDef NAND_Handler = {0};


static u8 read_status(void)
{
    vu8 data=0;
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_READSTA;//���Ͷ�״̬����
	data++;data++;data++;data++;data++;	//����ʱ,��ֹ-O2�Ż�,���µĴ���.
 	data=*(vu8*)NAND_ADDRESS;			//��ȡ״ֵ̬
    return data;
}
static u8 wait_ready(void)
{
    u8 status=0;
    vu32 time=0;
	while(1)						//�ȴ�ready
	{
		status=read_status();	//��ȡ״ֵ̬
		if(status&NSTA_READY)break;
		time++;
		if(time>=0X1FFFF)return NSTA_TIMEOUT;//��ʱ
	}
    return NSTA_READY;//׼����
}

static u8 nand_reset(void)
{
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_RESET;	//��λNAND
    if(wait_ready()==NSTA_READY)return 0;//��λ�ɹ�
    else return 1;								//��λʧ��
}

static u8 set_mode(u8 mode)
{
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_FEATURE;//����������������
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=0X01;		//��ַΪ0X01,����mode
 	*(vu8*)NAND_ADDRESS=mode;					//P1����,����mode
	*(vu8*)NAND_ADDRESS=0;
	*(vu8*)NAND_ADDRESS=0;
	*(vu8*)NAND_ADDRESS=0;
    if(wait_ready()==NSTA_READY)return 0;//�ɹ�
    else return 1;								//ʧ��
}
static u32 read_id(void)
{
    u8 deviceid[5];
    u32 id;
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_READID; //���Ͷ�ȡID����
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=0X00;
	//IDһ����5���ֽ�
    deviceid[0]=*(vu8*)NAND_ADDRESS;
    deviceid[1]=*(vu8*)NAND_ADDRESS;
    deviceid[2]=*(vu8*)NAND_ADDRESS;
    deviceid[3]=*(vu8*)NAND_ADDRESS;
    deviceid[4]=*(vu8*)NAND_ADDRESS;
    //þ���NAND FLASH��IDһ��5���ֽڣ�����Ϊ�˷�������ֻȡ4���ֽ����һ��32λ��IDֵ
    //����NAND FLASH�������ֲᣬֻҪ��þ���NAND FLASH����ôһ���ֽ�ID�ĵ�һ���ֽڶ���0X2C
    //�������ǾͿ����������0X2C��ֻȡ�������ֽڵ�IDֵ��
    id=((u32)deviceid[1])<<24|((u32)deviceid[2])<<16|((u32)deviceid[3])<<8|deviceid[4];
    return id;
}




static void nand_mpu_config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct = {0};

	HAL_MPU_Disable();											// ����MPU֮ǰ�ȹر�MPU

	// ����RAMΪregion4����СΪ256MB��������ɶ�д
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;					// ʹ��region
	MPU_InitStruct.Number = NAND_REGION_NUMBER;					// ����region��NANDʹ�õ�region4
	MPU_InitStruct.BaseAddress = NAND_ADDRESS_START;			// region����ַ
	MPU_InitStruct.Size = NAND_REGION_SIZE;						// region��С
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;	// ��region�ɶ�д
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;	// �����ȡ�������е�ָ��
	MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);						// ����MPU
}
void HAL_NAND_MspInit(NAND_HandleTypeDef *hnand)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_FMC_CLK_ENABLE();		// ʹ��FMCʱ��
    __HAL_RCC_GPIOD_CLK_ENABLE();	// ʹ��GPIODʱ��
    __HAL_RCC_GPIOE_CLK_ENABLE();	// ʹ��GPIOEʱ��
    __HAL_RCC_GPIOG_CLK_ENABLE();	// ʹ��GPIOGʱ��

	// PD13 NWP
	GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  		// �������
    GPIO_InitStruct.Pull = GPIO_NOPULL;					// û����/����
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  		// ����
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	// PD6 R/B����FMC_NWAIT
	GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;				// ����
    GPIO_InitStruct.Pull = GPIO_PULLUP;					// ����
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;	// ����
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	// PG9 FMC_NCE
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;			// ����ΪFMC
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    // PD0,1,4,5,11,12,14,15
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5| \
                          GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // PE7,8,9,10
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}


u8 nand_init(void)
{
    NAND_IDTypeDef nandID;
    FMC_NAND_PCC_TimingTypeDef ComSpaceTiming, AttSpaceTiming;

    nand_mpu_config();
    NAND_Handler.Instance=FMC_Bank3_R;
    NAND_Handler.Init.NandBank=FMC_NAND_BANK3;                          //NAND����BANK3��
    NAND_Handler.Init.Waitfeature=FMC_NAND_PCC_WAIT_FEATURE_DISABLE;    //�رյȴ�����
    NAND_Handler.Init.MemoryDataWidth=FMC_NAND_PCC_MEM_BUS_WIDTH_8;     //8λ���ݿ��
    NAND_Handler.Init.EccComputation=FMC_NAND_ECC_DISABLE;              //��ֹECC
    NAND_Handler.Init.ECCPageSize=FMC_NAND_ECC_PAGE_SIZE_512BYTE;       //ECCҳ��СΪ512�ֽ�
    NAND_Handler.Init.TCLRSetupTime=10;                                 //����TCLR(tCLR=CLE��RE����ʱ)=(TCLR+TSET+2)*THCLK,THCLK=1/200M=5ns
    NAND_Handler.Init.TARSetupTime=10;                                  //����TAR(tAR=ALE��RE����ʱ)=(TAR+TSET+1)*THCLK,THCLK=1/200M=5n��

	ComSpaceTiming.SetupTime=1;        //����ʱ��
    ComSpaceTiming.WaitSetupTime=3;    //�ȴ�ʱ��
    ComSpaceTiming.HoldSetupTime=2;    //����ʱ��
    ComSpaceTiming.HiZSetupTime=1;     //����̬ʱ��

    AttSpaceTiming.SetupTime=1;        //����ʱ��
    AttSpaceTiming.WaitSetupTime=3;    //�ȴ�ʱ��
    AttSpaceTiming.HoldSetupTime=2;    //����ʱ��
    AttSpaceTiming.HiZSetupTime=1;     //����̬ʱ��

    HAL_NAND_Init(&NAND_Handler,&ComSpaceTiming,&AttSpaceTiming);
    
    nand_reset();
    //HAL_NAND_Reset(&NAND_Handler);       		        //��λNAND
    delay_ms(100);

    nand_attr.id=read_id();	        //��ȡID
    //HAL_NAND_Read_ID(&NAND_Handler, &nandID);
    LOGD("NAND ID:%#x\r\n",nand_attr.id);

	set_mode(4);			        //����ΪMODE4,����ģʽ
    if(nand_attr.id==MT29F16G08ABABA) {   //NANDΪMT29F16G08ABABA
    
        nand_attr.page_totalsize=4320;
        nand_attr.page_mainsize=4096;
        nand_attr.page_sparesize=224;
        nand_attr.block_pagenum=128;
        nand_attr.plane_blocknum=2048;
        nand_attr.block_totalnum=4096;
    }
    else if(nand_attr.id==MT29F4G08ABADA) {//NANDΪMT29F4G08ABADA
    
        nand_attr.page_totalsize=2112;
        nand_attr.page_mainsize=2048;
        nand_attr.page_sparesize=64;
        nand_attr.block_pagenum=64;
        nand_attr.plane_blocknum=2048;
        nand_attr.block_totalnum=4096;
    }
	else
		return 1;	//���󣬷���

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);	// ȡ��д����

    return 0;
}




u8 nand_read_page(u32 PageNum,u16 ColNum,u8 *pBuffer,u16 NumByteToRead)
{
    U32 ecc[2];
    HAL_StatusTypeDef st;
    NAND_AddressTypeDef addr;
    
    addr.Block = (PageNum/nand_attr.block_pagenum)%nand_attr.plane_blocknum;
    addr.Page  = PageNum%nand_attr.block_pagenum;;
    addr.Plane = PageNum/(nand_attr.block_pagenum*nand_attr.plane_blocknum);
    
    //HAL_NAND_ECC_Enable(&NAND_Handler);
    st = HAL_NAND_Read_Page_8b(&NAND_Handler, &addr, pBuffer, NumByteToRead);
    //HAL_NAND_GetECC(&NAND_Handler, &ecc[0], HAL_MAX_DELAY);
    //HAL_NAND_ECC_Disable(&NAND_Handler);
    
    //HAL_NAND_Read_SpareArea_8b(&NAND_Handler, &addr, (uint8_t *)&ecc[1], 1);
    //if (ecc[0] != ecc[1]) {
    //    LOGE("ECC error!\n");
    //}
    
    return 0;
}



u8 nand_write_page(u32 PageNum,u16 ColNum,u8 *pBuffer,u16 NumByteToWrite)
{
    U32 ecc[2];
    HAL_StatusTypeDef st;
    NAND_AddressTypeDef addr;
    
    addr.Block = (PageNum/nand_attr.block_pagenum)%nand_attr.plane_blocknum;
    addr.Page  = PageNum%nand_attr.block_pagenum;;
    addr.Plane = PageNum/(nand_attr.block_pagenum*nand_attr.plane_blocknum);
    
    //HAL_NAND_ECC_Enable(&NAND_Handler);
    st = HAL_NAND_Write_Page_8b(&NAND_Handler, &addr, pBuffer, NumByteToWrite);
    //HAL_NAND_GetECC(&NAND_Handler, &ecc[0], HAL_MAX_DELAY);
    //HAL_NAND_ECC_Disable(&NAND_Handler);
    
    //HAL_NAND_Write_SpareArea_8b(&NAND_Handler, &addr, (uint8_t *)&ecc[1], 1);
    
    return 0;
}



u8 nand_erase_block(u32 BlockNum)
{
    HAL_StatusTypeDef st;
    NAND_AddressTypeDef addr;
    
    addr.Block = BlockNum%nand_attr.plane_blocknum;
    addr.Page = 0;
    addr.Plane = BlockNum/nand_attr.plane_blocknum;
    st = HAL_NAND_Erase_Block(&NAND_Handler, &addr);
    
    return 0;
}


nand_attr_t* nand_get_attr(void)
{
    return &nand_attr;
}




