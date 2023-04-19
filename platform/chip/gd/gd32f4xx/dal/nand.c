#include "platform.h"
#include "log.h"
#include "dal/nand.h"
#include "dal/delay.h"

typedef volatile uint8_t  vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;


//https://blog.csdn.net/u011996698/article/details/83545262



static nand_attr_t nand_attr;             	    // nand��Ҫ�����ṹ��
static NAND_HandleTypeDef NAND_Handler = {0};	// NAND FLASH���

static u8 nand_reset(void);
static u32 read_id(void);
static u8 set_mode(u8 mode);
static u8 wait_ready(void);
static u8 read_status(void);
static u8 ecc_correct(u8* data_buf,u32 eccrd,u32 ecccl);
static void nand_mpu_config(void);



//�ȴ�RB�ź�Ϊĳ����ƽ
//rb:0,�ȴ�RB==0
//   1,�ȴ�RB==1
//����ֵ:0,�ɹ�
//       1,��ʱ
static u8 wait_rb(vu8 rb)
{
    vu16 time=0;
	while(time<10000)
	{
		time++;
		if(NAND_RB==rb) return 0;
	}
	return 1;
}

//NAND��ʱ
static void nand_delay(vu32 i)
{
	while(i>0) i--;
}

//�ȴ�NAND׼����
//����ֵ:NSTA_TIMEOUT �ȴ���ʱ��
//      NSTA_READY    �Ѿ�׼����
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

//��λNAND
//����ֵ:0,�ɹ�;
//    ����,ʧ��
static u8 nand_reset(void)
{
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_RESET;	//��λNAND
    if(wait_ready()==NSTA_READY)return 0;//��λ�ɹ�
    else return 1;								//��λʧ��
}



//��ȡNAND FLASH��ID
//����ֵ:0,�ɹ�;
//    ����,ʧ��
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

//��ȡNAND FLASH��ID
//��ͬ��NAND���в�ͬ��������Լ���ʹ�õ�NAND FALSH�����ֲ�����д����
//����ֵ:NAND FLASH��IDֵ
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

//��NAND״̬
//����ֵ:NAND״ֵ̬
//bit0:0,�ɹ�;1,����(���/����/READ)
//bit6:0,Busy;1,Ready
static u8 read_status(void)
{
    vu8 data=0;
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_READSTA;//���Ͷ�״̬����
	data++;data++;data++;data++;data++;	//����ʱ,��ֹ-O2�Ż�,���µĴ���.
 	data=*(vu8*)NAND_ADDRESS;			//��ȡ״ֵ̬
    return data;
}


// NAND FLASH�ײ��������������ã�ʱ��ʹ��
// �˺����ᱻHAL_NAND_Init()����
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

//����MPU��region
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

//��ȡECC������λ/ż��λ
//oe:0,ż��λ
//   1,����λ
//eccval:�����eccֵ
//����ֵ:������eccֵ(���16λ)
static u16 get_ecc_oe(u8 oe,u32 eccval)
{
	u8 i;
	u16 ecctemp=0;
	for(i=0;i<24;i++)
	{
		if((i%2)==oe)
		{
			if((eccval>>i)&0X01)ecctemp+=1<<(i>>1);
		}
	}
	return ecctemp;
}

//ECCУ������
//eccrd:��ȡ����,ԭ�������ECCֵ
//ecccl:��ȡ����ʱ,Ӳ�������ECCֻ
//����ֵ:0,����������
//    ����,ECC����(�д���2��bit�Ĵ���,�޷��ָ�)
static u8 ecc_correct(u8* data_buf,u32 eccrd,u32 ecccl)
{
	u16 eccrdo,eccrde,eccclo,ecccle;
	u16 eccchk=0;
	u16 errorpos=0;
	u32 bytepos=0;
	eccrdo=get_ecc_oe(1,eccrd);	//��ȡeccrd������λ
	eccrde=get_ecc_oe(0,eccrd);	//��ȡeccrd��ż��λ
	eccclo=get_ecc_oe(1,ecccl);	//��ȡecccl������λ
	ecccle=get_ecc_oe(0,ecccl); 	//��ȡecccl��ż��λ
	eccchk=eccrdo^eccrde^eccclo^ecccle;
	if(eccchk==0XFFF)	//ȫ1,˵��ֻ��1bit ECC����
	{
		errorpos=eccrdo^eccclo;
        LOGD("errorpos:%d\r\n",errorpos);
		bytepos=errorpos/8;
		data_buf[bytepos]^=1<<(errorpos%8);
	}
	else				//����ȫ1,˵��������2bit ECC����,�޷��޸�
	{
		//LOGE("2bit ecc error or more\r\n");
		return 1;
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////



//��ʼ��NAND FLASH
u8 nand_init(void)
{
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
    nand_reset();       		        //��λNAND
    delay_ms(100);

    nand_attr.id=read_id();	        //��ȡID
    LOGD("NAND ID:%#x\r\n",nand_attr.id);

	set_mode(4);			        //����ΪMODE4,����ģʽ
    if(nand_attr.id==MT29F16G08ABABA)    //NANDΪMT29F16G08ABABA
    {
        nand_attr.page_totalsize=4320;
        nand_attr.page_mainsize=4096;
        nand_attr.page_sparesize=224;
        nand_attr.block_pagenum=128;
        nand_attr.plane_blocknum=2048;
        nand_attr.block_totalnum=4096;
    }
    else if(nand_attr.id==MT29F4G08ABADA)//NANDΪMT29F4G08ABADA
    {
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




//��ȡNAND Flash��ָ��ҳָ���е�����(main����spare��������ʹ�ô˺���)
//PageNum:Ҫ��ȡ��ҳ��ַ,��Χ:0~(block_pagenum*block_totalnum-1)
//ColNum:Ҫ��ȡ���п�ʼ��ַ(Ҳ����ҳ�ڵ�ַ),��Χ:0~(page_totalsize-1)
//*pBuffer:ָ�����ݴ洢��
//NumByteToRead:��ȡ�ֽ���(���ܿ�ҳ��)
//����ֵ:0,�ɹ�
//    ����,�������
u8 nand_read_page(u32 PageNum,u16 ColNum,u8 *pBuffer,u16 NumByteToRead)
{
    vu16 i=0;
	u8 res=0;
	u8 eccnum=0;		//��Ҫ�����ECC������ÿNAND_ECC_SECTOR_SIZE�ֽڼ���һ��ecc
	u8 eccstart=0;		//��һ��ECCֵ�����ĵ�ַ��Χ
	u8 errsta=0;
	u8 *p;
     *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_AREA_A;
    //���͵�ַ
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)ColNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(ColNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)PageNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>16);
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_AREA_TRUE1;
    //�������д����ǵȴ�R/B���ű�Ϊ�͵�ƽ����ʵ��Ҫ����ʱ���õģ��ȴ�NAND����R/B���š���Ϊ������ͨ��
    //��STM32��NWAIT����(NAND��R/B����)����Ϊ��ͨIO��������ͨ����ȡNWAIT���ŵĵ�ƽ���ж�NAND�Ƿ�׼��
    //�����ġ����Ҳ����ģ��ķ������������ٶȺܿ��ʱ���п���NAND��û���ü�����R/B��������ʾNAND��æ
    //��״̬��������ǾͶ�ȡ��R/B����,���ʱ��϶������ģ���ʵ��ȷʵ�ǻ����!���Ҳ���Խ���������
    //���뻻����ʱ����,ֻ������������Ϊ��Ч������û������ʱ������
	res=wait_rb(0);			//�ȴ�RB=0
    if(res)return NSTA_TIMEOUT;	//��ʱ�˳�
    //����2�д����������ж�NAND�Ƿ�׼���õ�
	res=wait_rb(1);			//�ȴ�RB=1
    if(res)return NSTA_TIMEOUT;	//��ʱ�˳�
	if(NumByteToRead%NAND_ECC_SECTOR_SIZE)//����NAND_ECC_SECTOR_SIZE����������������ECCУ��
	{
		//��ȡNAND FLASH�е�ֵ
		for(i=0;i<NumByteToRead;i++)
		{
			*(vu8*)pBuffer++ = *(vu8*)NAND_ADDRESS;
		}
	}else
	{
		eccnum=NumByteToRead/NAND_ECC_SECTOR_SIZE;			//�õ�ecc�������
		eccstart=ColNum/NAND_ECC_SECTOR_SIZE;
		p=pBuffer;
		for(res=0;res<eccnum;res++)
		{
			FMC_Bank3_R->PCR|=1<<6;						//ʹ��ECCУ��
			for(i=0;i<NAND_ECC_SECTOR_SIZE;i++)				//��ȡNAND_ECC_SECTOR_SIZE������
			{
				*(vu8*)pBuffer++ = *(vu8*)NAND_ADDRESS;
			}
			while(!(FMC_Bank3_R->SR&(1<<6)));				//�ȴ�FIFO��
			nand_attr.ecc_hdbuf[res+eccstart]=FMC_Bank3_R->ECCR;//��ȡӲ��������ECCֵ
			FMC_Bank3_R->PCR&=~(1<<6);						//��ֹECCУ��
		}
		i=nand_attr.page_mainsize+0X10+eccstart*4;			//��spare����0X10λ�ÿ�ʼ��ȡ֮ǰ�洢��eccֵ
		nand_delay(30);//�ȴ�tADL
		*(vu8*)(NAND_ADDRESS|NAND_CMD)=0X05;				//�����ָ��
		//���͵�ַ
		*(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)i;
		*(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(i>>8);
		*(vu8*)(NAND_ADDRESS|NAND_CMD)=0XE0;				//��ʼ������
		nand_delay(30);//�ȴ�tADL
		pBuffer=(u8*)&nand_attr.ecc_rdbuf[eccstart];
		for(i=0;i<4*eccnum;i++)								//��ȡ�����ECCֵ
		{
			*(vu8*)pBuffer++= *(vu8*)NAND_ADDRESS;
		}
		for(i=0;i<eccnum;i++)								//����ECC
		{
			if(nand_attr.ecc_rdbuf[i+eccstart]!=nand_attr.ecc_hdbuf[i+eccstart])//�����,��ҪУ��
			{
				//sprintf(printStr, "err hd,rd:0x%x,0x%x\r\n",nand_dev.ecc_hdbuf[i+eccstart],nand_dev.ecc_rdbuf[i+eccstart]);
                //LOGD(printStr);
 				//sprintf(printStr, "eccnum,eccstart:%d,%d\r\n",eccnum,eccstart);
                //DEBUG_PRINTF(printStr);
				//sprintf(printStr, "PageNum,ColNum:%d,%d\r\n",PageNum,ColNum);
                //DEBUG_PRINTF(printStr);

				res=ecc_correct(p+NAND_ECC_SECTOR_SIZE*i,nand_attr.ecc_rdbuf[i+eccstart],nand_attr.ecc_hdbuf[i+eccstart]);//ECCУ��
				if(res)errsta=NSTA_ECC2BITERR;				//���2BIT������ECC����
				else errsta=NSTA_ECC1BITERR;				//���1BIT ECC����
			}
		}
	}
    if(wait_ready()!=NSTA_READY)errsta=NSTA_ERROR;	//ʧ��
    return errsta;	//�ɹ�
}

//��ȡNAND Flash��ָ��ҳָ���е�����(main����spare��������ʹ�ô˺���),���Ա�(FTL����ʱ��Ҫ)
//PageNum:Ҫ��ȡ��ҳ��ַ,��Χ:0~(block_pagenum*block_totalnum-1)
//ColNum:Ҫ��ȡ���п�ʼ��ַ(Ҳ����ҳ�ڵ�ַ),��Χ:0~(page_totalsize-1)
//CmpVal:Ҫ�Աȵ�ֵ,��u32Ϊ��λ
//NumByteToRead:��ȡ����(��4�ֽ�Ϊ��λ,���ܿ�ҳ��)
//NumByteEqual:�ӳ�ʼλ�ó�����CmpValֵ��ͬ�����ݸ���
//����ֵ:0,�ɹ�
//    ����,�������
u8 nand_read_page_cmp(u32 PageNum,u16 ColNum,u32 CmpVal,u16 NumByteToRead,u16 *NumByteEqual)
{
    u16 i=0;
	u8 res=0;
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_AREA_A;
    //���͵�ַ
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)ColNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(ColNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)PageNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>16);
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_AREA_TRUE1;
    //�������д����ǵȴ�R/B���ű�Ϊ�͵�ƽ����ʵ��Ҫ����ʱ���õģ��ȴ�NAND����R/B���š���Ϊ������ͨ��
    //��STM32��NWAIT����(NAND��R/B����)����Ϊ��ͨIO��������ͨ����ȡNWAIT���ŵĵ�ƽ���ж�NAND�Ƿ�׼��
    //�����ġ����Ҳ����ģ��ķ������������ٶȺܿ��ʱ���п���NAND��û���ü�����R/B��������ʾNAND��æ
    //��״̬��������ǾͶ�ȡ��R/B����,���ʱ��϶������ģ���ʵ��ȷʵ�ǻ����!���Ҳ���Խ���������
    //���뻻����ʱ����,ֻ������������Ϊ��Ч������û������ʱ������
	res=wait_rb(0);			//�ȴ�RB=0
	if(res)return NSTA_TIMEOUT;	//��ʱ�˳�
    //����2�д����������ж�NAND�Ƿ�׼���õ�
	res=wait_rb(1);			//�ȴ�RB=1
    if(res)return NSTA_TIMEOUT;	//��ʱ�˳�
    for(i=0;i<NumByteToRead;i++)//��ȡ����,ÿ�ζ�4�ֽ�
    {
		if(*(vu32*)NAND_ADDRESS!=CmpVal)break;	//������κ�һ��ֵ,��CmpVal�����,���˳�.
    }
	*NumByteEqual=i;					//��CmpValֵ��ͬ�ĸ���
    if(wait_ready()!=NSTA_READY)return NSTA_ERROR;//ʧ��
    return 0;	//�ɹ�
}

//��NANDһҳ��д��ָ�����ֽڵ�����(main����spare��������ʹ�ô˺���)
//PageNum:Ҫд���ҳ��ַ,��Χ:0~(block_pagenum*block_totalnum-1)
//ColNum:Ҫд����п�ʼ��ַ(Ҳ����ҳ�ڵ�ַ),��Χ:0~(page_totalsize-1)
//pBbuffer:ָ�����ݴ洢��
//NumByteToWrite:Ҫд����ֽ�������ֵ���ܳ�����ҳʣ���ֽ���������
//����ֵ:0,�ɹ�
//    ����,�������
u8 nand_write_page(u32 PageNum,u16 ColNum,u8 *pBuffer,u16 NumByteToWrite)
{
    vu16 i=0;
	u8 res=0;
	u8 eccnum=0;		//��Ҫ�����ECC������ÿNAND_ECC_SECTOR_SIZE�ֽڼ���һ��ecc
	u8 eccstart=0;		//��һ��ECCֵ�����ĵ�ַ��Χ

	*(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_WRITE0;
    //���͵�ַ
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)ColNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(ColNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)PageNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>16);
	nand_delay(30);//�ȴ�tADL
	if(NumByteToWrite%NAND_ECC_SECTOR_SIZE)//����NAND_ECC_SECTOR_SIZE����������������ECCУ��
	{
		for(i=0;i<NumByteToWrite;i++)		//д������
		{
			*(vu8*)NAND_ADDRESS=*(vu8*)pBuffer++;
		}
	}else
	{
		eccnum=NumByteToWrite/NAND_ECC_SECTOR_SIZE;			//�õ�ecc�������
		eccstart=ColNum/NAND_ECC_SECTOR_SIZE;
 		for(res=0;res<eccnum;res++)
		{
			FMC_Bank3_R->PCR|=1<<6;						//ʹ��ECCУ��
			for(i=0;i<NAND_ECC_SECTOR_SIZE;i++)				//д��NAND_ECC_SECTOR_SIZE������
			{
				*(vu8*)NAND_ADDRESS=*(vu8*)pBuffer++;
			}
			while(!(FMC_Bank3_R->SR&(1<<6)));				//�ȴ�FIFO��
			nand_attr.ecc_hdbuf[res+eccstart]=FMC_Bank3_R->ECCR;	//��ȡӲ��������ECCֵ
  			FMC_Bank3_R->PCR&=~(1<<6);						//��ֹECCУ��
		}
		i=nand_attr.page_mainsize+0X10+eccstart*4;			//����д��ECC��spare����ַ
		nand_delay(30);//�ȴ�
		*(vu8*)(NAND_ADDRESS|NAND_CMD)=0X85;				//���дָ��
		//���͵�ַ
		*(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)i;
		*(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(i>>8);
		nand_delay(30);//�ȴ�tADL
		pBuffer=(u8*)&nand_attr.ecc_hdbuf[eccstart];
		for(i=0;i<eccnum;i++)					//д��ECC
		{
			for(res=0;res<4;res++)
			{
				*(vu8*)NAND_ADDRESS=*(vu8*)pBuffer++;
			}
		}
	}
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_WRITE_TURE1;
    if(wait_ready()!=NSTA_READY)return NSTA_ERROR;//ʧ��
    return 0;//�ɹ�
}

//��NANDһҳ�е�ָ����ַ��ʼ,д��ָ�����ȵĺ㶨����
//PageNum:Ҫд���ҳ��ַ,��Χ:0~(block_pagenum*block_totalnum-1)
//ColNum:Ҫд����п�ʼ��ַ(Ҳ����ҳ�ڵ�ַ),��Χ:0~(page_totalsize-1)
//cval:Ҫд���ָ������
//NumByteToWrite:Ҫд�������(��4�ֽ�Ϊ��λ)
//����ֵ:0,�ɹ�
//    ����,�������
u8 nand_page_set_value(u32 PageNum,u16 ColNum,u32 cval,u16 NumByteToWrite)
{
    u16 i=0;
	*(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_WRITE0;
    //���͵�ַ
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)ColNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(ColNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)PageNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>16);
	nand_delay(30);//�ȴ�tADL
	for(i=0;i<NumByteToWrite;i++)		//д������,ÿ��д4�ֽ�
	{
		*(vu32*)NAND_ADDRESS=cval;
	}
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_WRITE_TURE1;
    if(wait_ready()!=NSTA_READY)return NSTA_ERROR;//ʧ��
    return 0;//�ɹ�
}



//��ȡspare���е�����
//PageNum:Ҫд���ҳ��ַ,��Χ:0~(block_pagenum*block_totalnum-1)
//ColNum:Ҫд���spare����ַ(spare�����ĸ���ַ),��Χ:0~(page_sparesize-1)
//pBuffer:�������ݻ�����
//NumByteToRead:Ҫ��ȡ���ֽ���(������page_sparesize)
//����ֵ:0,�ɹ�
//    ����,�������
u8 nand_read_spare(u32 PageNum,u16 ColNum,u8 *pBuffer,u16 NumByteToRead)
{
    u8 temp=0;
    u8 remainbyte=0;
    remainbyte=nand_attr.page_sparesize-ColNum;
    if(NumByteToRead>remainbyte) NumByteToRead=remainbyte;  //ȷ��Ҫд����ֽ���������spareʣ��Ĵ�С
    temp=nand_read_page(PageNum,ColNum+nand_attr.page_mainsize,pBuffer,NumByteToRead);//��ȡ����
    return temp;
}

//��spare����д����
//PageNum:Ҫд���ҳ��ַ,��Χ:0~(block_pagenum*block_totalnum-1)
//ColNum:Ҫд���spare����ַ(spare�����ĸ���ַ),��Χ:0~(page_sparesize-1)
//pBuffer:Ҫд��������׵�ַ
//NumByteToWrite:Ҫд����ֽ���(������page_sparesize)
//����ֵ:0,�ɹ�
//    ����,ʧ��
u8 nand_write_spare(u32 PageNum,u16 ColNum,u8 *pBuffer,u16 NumByteToWrite)
{
    u8 temp=0;
    u8 remainbyte=0;
    remainbyte=nand_attr.page_sparesize-ColNum;
    if(NumByteToWrite>remainbyte) NumByteToWrite=remainbyte;  //ȷ��Ҫ��ȡ���ֽ���������spareʣ��Ĵ�С
    temp=nand_write_page(PageNum,ColNum+nand_attr.page_mainsize,pBuffer,NumByteToWrite);//��ȡ
    return temp;
}

//����һ����
//BlockNum:Ҫ������BLOCK���,��Χ:0-(block_totalnum-1)
//����ֵ:0,�����ɹ�
//    ����,����ʧ��
u8 nand_erase_block(u32 BlockNum)
{
	if(nand_attr.id==MT29F16G08ABABA)BlockNum<<=7;  	//�����ַת��Ϊҳ��ַ
    else if(nand_attr.id==MT29F4G08ABADA)BlockNum<<=6;
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_ERASE0;
    //���Ϳ��ַ
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)BlockNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(BlockNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(BlockNum>>16);
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_ERASE1;
	if(wait_ready()!=NSTA_READY)return NSTA_ERROR;//ʧ��
    return 0;	//�ɹ�
}

//ȫƬ����NAND FLASH
void nand_erase_chip(void)
{
    u8 status;
    u16 i=0;
    for(i=0;i<nand_attr.block_totalnum;i++) //ѭ���������еĿ�
    {
        status=nand_erase_block(i);
        if(status)
        {
            LOGE("Erase %d block fail!!��������Ϊ%d\r\n",i,status);//����ʧ��
        }
    }
}



nand_attr_t* nand_get_attr(void)
{
    return &nand_attr;
}




