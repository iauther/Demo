#include "platform.h"
#include "log.h"
#include "dal_nand.h"
#include "dal_delay.h"

//eccУ�顢�����жϼ��궨������
//https://blog.csdn.net/sudaroot/article/details/99640553


typedef volatile uint8_t  vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;

static void nand_test(void);


static NAND_HandleTypeDef NAND_Handler = {0};
static dal_nand_para_t NandParam={
    .bytes_per_page = 2048,
	.bytes_per_spare = 64,
	.pages_per_block = 64,
	.blocks_per_plane = 2048,
	.planes_per_chip = 2,
};

static u8 read_status(void)
{
    vu8 data=0;
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_READSTA;//���Ͷ�״̬����
	data++;data++;data++;data++;data++;	//����ʱ,��ֹ-O2�Ż�,���µĴ���.
 	data=*(vu8*)NAND_ADDRESS;			//��ȡ״ֵ̬
    return data;
}
static int wait_ready(void)
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

static int nand_reset(void)
{
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_RESET;	//��λNAND
    if(wait_ready()==NSTA_READY)return 0;//��λ�ɹ�
    else return 1;								//��λʧ��
}

static int set_mode(u8 mode)
{
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_FEATURE;//����������������
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=0X01;		//��ַΪ0X01,����mode
 	*(vu8*)NAND_ADDRESS=mode;					//P1����,����mode
	*(vu8*)NAND_ADDRESS=0;
	*(vu8*)NAND_ADDRESS=0;
	*(vu8*)NAND_ADDRESS=0;
    if(wait_ready()==NSTA_READY)
        return 0;//�ɹ�
    else
        return 1;								//ʧ��
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


int dal_nand_init(void)
{
    HAL_StatusTypeDef st;
    NAND_IDTypeDef nandID;
    NAND_DeviceConfigTypeDef nandCfg;
    FMC_NAND_PCC_TimingTypeDef ComSpaceTiming, AttSpaceTiming;

    nand_mpu_config();
    
    NAND_Handler.Instance = FMC_Bank3_R;
    NAND_Handler.Init.NandBank = FMC_NAND_BANK3;                          //NAND����BANK3��
    NAND_Handler.Init.Waitfeature = FMC_NAND_PCC_WAIT_FEATURE_DISABLE;    //�رյȴ�����
    NAND_Handler.Init.MemoryDataWidth = FMC_NAND_PCC_MEM_BUS_WIDTH_8;     //8λ���ݿ��
    NAND_Handler.Init.EccComputation = FMC_NAND_ECC_DISABLE;              //��ֹECC
    NAND_Handler.Init.ECCPageSize = FMC_NAND_ECC_PAGE_SIZE_512BYTE;       //ECCҳ��СΪ512�ֽ�
    NAND_Handler.Init.TCLRSetupTime = 10;                                 //����TCLR(tCLR=CLE��RE����ʱ)=(TCLR+TSET+2)*THCLK,THCLK=1/200M=5ns
    NAND_Handler.Init.TARSetupTime = 10;                                  //����TAR(tAR=ALE��RE����ʱ)=(TAR+TSET+1)*THCLK,THCLK=1/200M=5n��

    NAND_Handler.Config.PageSize = NandParam.bytes_per_page;
	NAND_Handler.Config.SpareAreaSize = NandParam.bytes_per_spare;
	NAND_Handler.Config.BlockSize = NandParam.pages_per_block;
	NAND_Handler.Config.BlockNbr = NandParam.blocks_per_plane*NandParam.planes_per_chip;
	NAND_Handler.Config.PlaneNbr = NandParam.planes_per_chip;
	NAND_Handler.Config.PlaneSize = NandParam.blocks_per_plane;
	NAND_Handler.Config.ExtraCommandEnable = ENABLE;
    
	ComSpaceTiming.SetupTime = 1;        //����ʱ��
    ComSpaceTiming.WaitSetupTime = 3;    //�ȴ�ʱ��
    ComSpaceTiming.HoldSetupTime = 2;    //����ʱ��
    ComSpaceTiming.HiZSetupTime = 1;     //����̬ʱ��

    AttSpaceTiming.SetupTime = 1;        //����ʱ��
    AttSpaceTiming.WaitSetupTime = 3;    //�ȴ�ʱ��
    AttSpaceTiming.HoldSetupTime = 2;    //����ʱ��
    AttSpaceTiming.HiZSetupTime = 1;     //����̬ʱ��


    st = HAL_NAND_Init(&NAND_Handler,&ComSpaceTiming,&AttSpaceTiming);
    dal_delay_ms(100);
    
    st = HAL_NAND_Reset(&NAND_Handler);       		        //��λNAND
    dal_delay_ms(100);
    
    st = HAL_NAND_Read_ID(&NAND_Handler, &nandID);
    LOGD("NAND ID: 0x%02x%02x%02x%02x\r\n",nandID.Maker_Id, 
                                           nandID.Device_Id, 
                                           nandID.Third_Id, 
                                           nandID.Fourth_Id);

	set_mode(4);			                                //����ΪMODE4,����ģʽ
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);	// ȡ��д����

    //nand_test();

    return 0;
}




int dal_nand_read_page(u32 page, u8 *data, int data_len, u8 *oob, int oob_len)
{
    U32 ecc[2];
    HAL_StatusTypeDef st;
    NAND_AddressTypeDef addr;
    
    addr.Block = (page/NandParam.pages_per_block)%NandParam.blocks_per_plane;
    addr.Page  = page%NandParam.pages_per_block;
    addr.Plane = page/(NandParam.pages_per_block*NandParam.blocks_per_plane);
    
    //HAL_NAND_ECC_Enable(&NAND_Handler);
    if(data && data_len) {
        st = HAL_NAND_Read_Page_8b(&NAND_Handler, &addr, data, 1);
    }
    //HAL_NAND_GetECC(&NAND_Handler, &ecc[0], HAL_MAX_DELAY);
    //HAL_NAND_ECC_Disable(&NAND_Handler);
    
    if(oob && oob_len) {
        HAL_NAND_Read_SpareArea_8b(&NAND_Handler, &addr, oob, 1);
    }
    
    return 0;
}



int dal_nand_write_page(u32 page, u8 *data, int data_len, u8 *oob, int oob_len)
{
    U32 ecc[2];
    HAL_StatusTypeDef st;
    NAND_AddressTypeDef addr;
    
    addr.Block = (page/NandParam.pages_per_block)%NandParam.blocks_per_plane;
    addr.Page  = page%NandParam.pages_per_block;
    addr.Plane = page/(NandParam.pages_per_block*NandParam.blocks_per_plane);
    
    //HAL_NAND_ECC_Enable(&NAND_Handler);
    if(data && data_len) {
        st = HAL_NAND_Write_Page_8b(&NAND_Handler, &addr, data, 1);
    }
    //HAL_NAND_GetECC(&NAND_Handler, &ecc[0], HAL_MAX_DELAY);
    //HAL_NAND_ECC_Disable(&NAND_Handler);
    
    if(oob && oob_len) {
        HAL_NAND_Write_SpareArea_8b(&NAND_Handler, &addr, oob, 1);
    }
    
    return 0;
}



int dal_nand_erase_block(u32 block)
{
    int s;
    HAL_StatusTypeDef st;
    NAND_AddressTypeDef addr;
    
    addr.Block = block%NandParam.blocks_per_plane;
    addr.Page  = 0;
    addr.Plane = block/NandParam.blocks_per_plane;
    
    st = HAL_NAND_Erase_Block(&NAND_Handler, &addr);
    if(wait_ready() !=NSTA_READY) {
        return -1;
    }
    
    return 0;
}



dal_nand_para_t *dal_nand_get_para(void)
{
    return &NandParam;
}



#include "log.h"
#if 0
u8 tmp[2048]={0};
u8 oob[64]={0xff};
u8 oob2[64]={0x00};
void nand_test(void)
{
    int i,j=0;
    
    //dal_nand_erase_block(0);
    
    for(i=0; i<2048; i++) {
        tmp[i] = i;
    }
    //dal_nand_write_page(0, tmp, 1, oob, 1);
    
    memset(tmp, 0, sizeof(tmp));
    dal_nand_read_page(0, tmp, 1, oob2, 1);
    j++;
}
#endif

