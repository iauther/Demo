#include "platform.h"
#include "log.h"
#include "dal/nand.h"
#include "dal/delay.h"

typedef volatile uint8_t  vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;


//https://blog.csdn.net/u011996698/article/details/83545262



static nand_attr_t nand_attr;             	    // nand重要参数结构体
static NAND_HandleTypeDef NAND_Handler = {0};	// NAND FLASH句柄

static u8 nand_reset(void);
static u32 read_id(void);
static u8 set_mode(u8 mode);
static u8 wait_ready(void);
static u8 read_status(void);
static u8 ecc_correct(u8* data_buf,u32 eccrd,u32 ecccl);
static void nand_mpu_config(void);



//等待RB信号为某个电平
//rb:0,等待RB==0
//   1,等待RB==1
//返回值:0,成功
//       1,超时
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

//NAND延时
static void nand_delay(vu32 i)
{
	while(i>0) i--;
}

//等待NAND准备好
//返回值:NSTA_TIMEOUT 等待超时了
//      NSTA_READY    已经准备好
static u8 wait_ready(void)
{
    u8 status=0;
    vu32 time=0;
	while(1)						//等待ready
	{
		status=read_status();	//获取状态值
		if(status&NSTA_READY)break;
		time++;
		if(time>=0X1FFFF)return NSTA_TIMEOUT;//超时
	}
    return NSTA_READY;//准备好
}

//复位NAND
//返回值:0,成功;
//    其他,失败
static u8 nand_reset(void)
{
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_RESET;	//复位NAND
    if(wait_ready()==NSTA_READY)return 0;//复位成功
    else return 1;								//复位失败
}



//读取NAND FLASH的ID
//返回值:0,成功;
//    其他,失败
static u8 set_mode(u8 mode)
{
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_FEATURE;//发送设置特性命令
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=0X01;		//地址为0X01,设置mode
 	*(vu8*)NAND_ADDRESS=mode;					//P1参数,设置mode
	*(vu8*)NAND_ADDRESS=0;
	*(vu8*)NAND_ADDRESS=0;
	*(vu8*)NAND_ADDRESS=0;
    if(wait_ready()==NSTA_READY)return 0;//成功
    else return 1;								//失败
}

//读取NAND FLASH的ID
//不同的NAND略有不同，请根据自己所使用的NAND FALSH数据手册来编写函数
//返回值:NAND FLASH的ID值
static u32 read_id(void)
{
    u8 deviceid[5];
    u32 id;
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_READID; //发送读取ID命令
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=0X00;
	//ID一共有5个字节
    deviceid[0]=*(vu8*)NAND_ADDRESS;
    deviceid[1]=*(vu8*)NAND_ADDRESS;
    deviceid[2]=*(vu8*)NAND_ADDRESS;
    deviceid[3]=*(vu8*)NAND_ADDRESS;
    deviceid[4]=*(vu8*)NAND_ADDRESS;
    //镁光的NAND FLASH的ID一共5个字节，但是为了方便我们只取4个字节组成一个32位的ID值
    //根据NAND FLASH的数据手册，只要是镁光的NAND FLASH，那么一个字节ID的第一个字节都是0X2C
    //所以我们就可以抛弃这个0X2C，只取后面四字节的ID值。
    id=((u32)deviceid[1])<<24|((u32)deviceid[2])<<16|((u32)deviceid[3])<<8|deviceid[4];
    return id;
}

//读NAND状态
//返回值:NAND状态值
//bit0:0,成功;1,错误(编程/擦除/READ)
//bit6:0,Busy;1,Ready
static u8 read_status(void)
{
    vu8 data=0;
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_READSTA;//发送读状态命令
	data++;data++;data++;data++;data++;	//加延时,防止-O2优化,导致的错误.
 	data=*(vu8*)NAND_ADDRESS;			//读取状态值
    return data;
}


// NAND FLASH底层驱动，引脚配置，时钟使能
// 此函数会被HAL_NAND_Init()调用
void HAL_NAND_MspInit(NAND_HandleTypeDef *hnand)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_FMC_CLK_ENABLE();		// 使能FMC时钟
    __HAL_RCC_GPIOD_CLK_ENABLE();	// 使能GPIOD时钟
    __HAL_RCC_GPIOE_CLK_ENABLE();	// 使能GPIOE时钟
    __HAL_RCC_GPIOG_CLK_ENABLE();	// 使能GPIOG时钟

	// PD13 NWP
	GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  		// 推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;					// 没有上/下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  		// 低速
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	// PD6 R/B引脚FMC_NWAIT
	GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;				// 输入
    GPIO_InitStruct.Pull = GPIO_PULLUP;					// 上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;	// 高速
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	// PG9 FMC_NCE
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF12_FMC;			// 复用为FMC
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

//配置MPU的region
static void nand_mpu_config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct = {0};

	HAL_MPU_Disable();											// 配置MPU之前先关闭MPU

	// 配置RAM为region4，大小为256MB，此区域可读写
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;					// 使能region
	MPU_InitStruct.Number = NAND_REGION_NUMBER;					// 设置region，NAND使用的region4
	MPU_InitStruct.BaseAddress = NAND_ADDRESS_START;			// region基地址
	MPU_InitStruct.Size = NAND_REGION_SIZE;						// region大小
	MPU_InitStruct.SubRegionDisable = 0x00;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;	// 此region可读写
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;	// 允许读取此区域中的指令
	MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);						// 开启MPU
}

//获取ECC的奇数位/偶数位
//oe:0,偶数位
//   1,奇数位
//eccval:输入的ecc值
//返回值:计算后的ecc值(最多16位)
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

//ECC校正函数
//eccrd:读取出来,原来保存的ECC值
//ecccl:读取数据时,硬件计算的ECC只
//返回值:0,错误已修正
//    其他,ECC错误(有大于2个bit的错误,无法恢复)
static u8 ecc_correct(u8* data_buf,u32 eccrd,u32 ecccl)
{
	u16 eccrdo,eccrde,eccclo,ecccle;
	u16 eccchk=0;
	u16 errorpos=0;
	u32 bytepos=0;
	eccrdo=get_ecc_oe(1,eccrd);	//获取eccrd的奇数位
	eccrde=get_ecc_oe(0,eccrd);	//获取eccrd的偶数位
	eccclo=get_ecc_oe(1,ecccl);	//获取ecccl的奇数位
	ecccle=get_ecc_oe(0,ecccl); 	//获取ecccl的偶数位
	eccchk=eccrdo^eccrde^eccclo^ecccle;
	if(eccchk==0XFFF)	//全1,说明只有1bit ECC错误
	{
		errorpos=eccrdo^eccclo;
        LOGD("errorpos:%d\r\n",errorpos);
		bytepos=errorpos/8;
		data_buf[bytepos]^=1<<(errorpos%8);
	}
	else				//不是全1,说明至少有2bit ECC错误,无法修复
	{
		//LOGE("2bit ecc error or more\r\n");
		return 1;
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////



//初始化NAND FLASH
u8 nand_init(void)
{
    FMC_NAND_PCC_TimingTypeDef ComSpaceTiming, AttSpaceTiming;

    nand_mpu_config();
    NAND_Handler.Instance=FMC_Bank3_R;
    NAND_Handler.Init.NandBank=FMC_NAND_BANK3;                          //NAND挂在BANK3上
    NAND_Handler.Init.Waitfeature=FMC_NAND_PCC_WAIT_FEATURE_DISABLE;    //关闭等待特性
    NAND_Handler.Init.MemoryDataWidth=FMC_NAND_PCC_MEM_BUS_WIDTH_8;     //8位数据宽度
    NAND_Handler.Init.EccComputation=FMC_NAND_ECC_DISABLE;              //禁止ECC
    NAND_Handler.Init.ECCPageSize=FMC_NAND_ECC_PAGE_SIZE_512BYTE;       //ECC页大小为512字节
    NAND_Handler.Init.TCLRSetupTime=10;                                 //设置TCLR(tCLR=CLE到RE的延时)=(TCLR+TSET+2)*THCLK,THCLK=1/200M=5ns
    NAND_Handler.Init.TARSetupTime=10;                                  //设置TAR(tAR=ALE到RE的延时)=(TAR+TSET+1)*THCLK,THCLK=1/200M=5n。

	ComSpaceTiming.SetupTime=1;        //建立时间
    ComSpaceTiming.WaitSetupTime=3;    //等待时间
    ComSpaceTiming.HoldSetupTime=2;    //保持时间
    ComSpaceTiming.HiZSetupTime=1;     //高阻态时间

    AttSpaceTiming.SetupTime=1;        //建立时间
    AttSpaceTiming.WaitSetupTime=3;    //等待时间
    AttSpaceTiming.HoldSetupTime=2;    //保持时间
    AttSpaceTiming.HiZSetupTime=1;     //高阻态时间

    HAL_NAND_Init(&NAND_Handler,&ComSpaceTiming,&AttSpaceTiming);
    nand_reset();       		        //复位NAND
    delay_ms(100);

    nand_attr.id=read_id();	        //读取ID
    LOGD("NAND ID:%#x\r\n",nand_attr.id);

	set_mode(4);			        //设置为MODE4,高速模式
    if(nand_attr.id==MT29F16G08ABABA)    //NAND为MT29F16G08ABABA
    {
        nand_attr.page_totalsize=4320;
        nand_attr.page_mainsize=4096;
        nand_attr.page_sparesize=224;
        nand_attr.block_pagenum=128;
        nand_attr.plane_blocknum=2048;
        nand_attr.block_totalnum=4096;
    }
    else if(nand_attr.id==MT29F4G08ABADA)//NAND为MT29F4G08ABADA
    {
        nand_attr.page_totalsize=2112;
        nand_attr.page_mainsize=2048;
        nand_attr.page_sparesize=64;
        nand_attr.block_pagenum=64;
        nand_attr.plane_blocknum=2048;
        nand_attr.block_totalnum=4096;
    }
	else
		return 1;	//错误，返回

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);	// 取消写保护

    return 0;
}




//读取NAND Flash的指定页指定列的数据(main区和spare区都可以使用此函数)
//PageNum:要读取的页地址,范围:0~(block_pagenum*block_totalnum-1)
//ColNum:要读取的列开始地址(也就是页内地址),范围:0~(page_totalsize-1)
//*pBuffer:指向数据存储区
//NumByteToRead:读取字节数(不能跨页读)
//返回值:0,成功
//    其他,错误代码
u8 nand_read_page(u32 PageNum,u16 ColNum,u8 *pBuffer,u16 NumByteToRead)
{
    vu16 i=0;
	u8 res=0;
	u8 eccnum=0;		//需要计算的ECC个数，每NAND_ECC_SECTOR_SIZE字节计算一个ecc
	u8 eccstart=0;		//第一个ECC值所属的地址范围
	u8 errsta=0;
	u8 *p;
     *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_AREA_A;
    //发送地址
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)ColNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(ColNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)PageNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>16);
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_AREA_TRUE1;
    //下面两行代码是等待R/B引脚变为低电平，其实主要起延时作用的，等待NAND操作R/B引脚。因为我们是通过
    //将STM32的NWAIT引脚(NAND的R/B引脚)配置为普通IO，代码中通过读取NWAIT引脚的电平来判断NAND是否准备
    //就绪的。这个也就是模拟的方法，所以在速度很快的时候有可能NAND还没来得及操作R/B引脚来表示NAND的忙
    //闲状态，结果我们就读取了R/B引脚,这个时候肯定会出错的，事实上确实是会出错!大家也可以将下面两行
    //代码换成延时函数,只不过这里我们为了效率所以没有用延时函数。
	res=wait_rb(0);			//等待RB=0
    if(res)return NSTA_TIMEOUT;	//超时退出
    //下面2行代码是真正判断NAND是否准备好的
	res=wait_rb(1);			//等待RB=1
    if(res)return NSTA_TIMEOUT;	//超时退出
	if(NumByteToRead%NAND_ECC_SECTOR_SIZE)//不是NAND_ECC_SECTOR_SIZE的整数倍，不进行ECC校验
	{
		//读取NAND FLASH中的值
		for(i=0;i<NumByteToRead;i++)
		{
			*(vu8*)pBuffer++ = *(vu8*)NAND_ADDRESS;
		}
	}else
	{
		eccnum=NumByteToRead/NAND_ECC_SECTOR_SIZE;			//得到ecc计算次数
		eccstart=ColNum/NAND_ECC_SECTOR_SIZE;
		p=pBuffer;
		for(res=0;res<eccnum;res++)
		{
			FMC_Bank3_R->PCR|=1<<6;						//使能ECC校验
			for(i=0;i<NAND_ECC_SECTOR_SIZE;i++)				//读取NAND_ECC_SECTOR_SIZE个数据
			{
				*(vu8*)pBuffer++ = *(vu8*)NAND_ADDRESS;
			}
			while(!(FMC_Bank3_R->SR&(1<<6)));				//等待FIFO空
			nand_attr.ecc_hdbuf[res+eccstart]=FMC_Bank3_R->ECCR;//读取硬件计算后的ECC值
			FMC_Bank3_R->PCR&=~(1<<6);						//禁止ECC校验
		}
		i=nand_attr.page_mainsize+0X10+eccstart*4;			//从spare区的0X10位置开始读取之前存储的ecc值
		nand_delay(30);//等待tADL
		*(vu8*)(NAND_ADDRESS|NAND_CMD)=0X05;				//随机读指令
		//发送地址
		*(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)i;
		*(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(i>>8);
		*(vu8*)(NAND_ADDRESS|NAND_CMD)=0XE0;				//开始读数据
		nand_delay(30);//等待tADL
		pBuffer=(u8*)&nand_attr.ecc_rdbuf[eccstart];
		for(i=0;i<4*eccnum;i++)								//读取保存的ECC值
		{
			*(vu8*)pBuffer++= *(vu8*)NAND_ADDRESS;
		}
		for(i=0;i<eccnum;i++)								//检验ECC
		{
			if(nand_attr.ecc_rdbuf[i+eccstart]!=nand_attr.ecc_hdbuf[i+eccstart])//不相等,需要校正
			{
				//sprintf(printStr, "err hd,rd:0x%x,0x%x\r\n",nand_dev.ecc_hdbuf[i+eccstart],nand_dev.ecc_rdbuf[i+eccstart]);
                //LOGD(printStr);
 				//sprintf(printStr, "eccnum,eccstart:%d,%d\r\n",eccnum,eccstart);
                //DEBUG_PRINTF(printStr);
				//sprintf(printStr, "PageNum,ColNum:%d,%d\r\n",PageNum,ColNum);
                //DEBUG_PRINTF(printStr);

				res=ecc_correct(p+NAND_ECC_SECTOR_SIZE*i,nand_attr.ecc_rdbuf[i+eccstart],nand_attr.ecc_hdbuf[i+eccstart]);//ECC校验
				if(res)errsta=NSTA_ECC2BITERR;				//标记2BIT及以上ECC错误
				else errsta=NSTA_ECC1BITERR;				//标记1BIT ECC错误
			}
		}
	}
    if(wait_ready()!=NSTA_READY)errsta=NSTA_ERROR;	//失败
    return errsta;	//成功
}

//读取NAND Flash的指定页指定列的数据(main区和spare区都可以使用此函数),并对比(FTL管理时需要)
//PageNum:要读取的页地址,范围:0~(block_pagenum*block_totalnum-1)
//ColNum:要读取的列开始地址(也就是页内地址),范围:0~(page_totalsize-1)
//CmpVal:要对比的值,以u32为单位
//NumByteToRead:读取字数(以4字节为单位,不能跨页读)
//NumByteEqual:从初始位置持续与CmpVal值相同的数据个数
//返回值:0,成功
//    其他,错误代码
u8 nand_read_page_cmp(u32 PageNum,u16 ColNum,u32 CmpVal,u16 NumByteToRead,u16 *NumByteEqual)
{
    u16 i=0;
	u8 res=0;
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_AREA_A;
    //发送地址
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)ColNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(ColNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)PageNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>16);
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_AREA_TRUE1;
    //下面两行代码是等待R/B引脚变为低电平，其实主要起延时作用的，等待NAND操作R/B引脚。因为我们是通过
    //将STM32的NWAIT引脚(NAND的R/B引脚)配置为普通IO，代码中通过读取NWAIT引脚的电平来判断NAND是否准备
    //就绪的。这个也就是模拟的方法，所以在速度很快的时候有可能NAND还没来得及操作R/B引脚来表示NAND的忙
    //闲状态，结果我们就读取了R/B引脚,这个时候肯定会出错的，事实上确实是会出错!大家也可以将下面两行
    //代码换成延时函数,只不过这里我们为了效率所以没有用延时函数。
	res=wait_rb(0);			//等待RB=0
	if(res)return NSTA_TIMEOUT;	//超时退出
    //下面2行代码是真正判断NAND是否准备好的
	res=wait_rb(1);			//等待RB=1
    if(res)return NSTA_TIMEOUT;	//超时退出
    for(i=0;i<NumByteToRead;i++)//读取数据,每次读4字节
    {
		if(*(vu32*)NAND_ADDRESS!=CmpVal)break;	//如果有任何一个值,与CmpVal不相等,则退出.
    }
	*NumByteEqual=i;					//与CmpVal值相同的个数
    if(wait_ready()!=NSTA_READY)return NSTA_ERROR;//失败
    return 0;	//成功
}

//在NAND一页中写入指定个字节的数据(main区和spare区都可以使用此函数)
//PageNum:要写入的页地址,范围:0~(block_pagenum*block_totalnum-1)
//ColNum:要写入的列开始地址(也就是页内地址),范围:0~(page_totalsize-1)
//pBbuffer:指向数据存储区
//NumByteToWrite:要写入的字节数，该值不能超过该页剩余字节数！！！
//返回值:0,成功
//    其他,错误代码
u8 nand_write_page(u32 PageNum,u16 ColNum,u8 *pBuffer,u16 NumByteToWrite)
{
    vu16 i=0;
	u8 res=0;
	u8 eccnum=0;		//需要计算的ECC个数，每NAND_ECC_SECTOR_SIZE字节计算一个ecc
	u8 eccstart=0;		//第一个ECC值所属的地址范围

	*(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_WRITE0;
    //发送地址
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)ColNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(ColNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)PageNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>16);
	nand_delay(30);//等待tADL
	if(NumByteToWrite%NAND_ECC_SECTOR_SIZE)//不是NAND_ECC_SECTOR_SIZE的整数倍，不进行ECC校验
	{
		for(i=0;i<NumByteToWrite;i++)		//写入数据
		{
			*(vu8*)NAND_ADDRESS=*(vu8*)pBuffer++;
		}
	}else
	{
		eccnum=NumByteToWrite/NAND_ECC_SECTOR_SIZE;			//得到ecc计算次数
		eccstart=ColNum/NAND_ECC_SECTOR_SIZE;
 		for(res=0;res<eccnum;res++)
		{
			FMC_Bank3_R->PCR|=1<<6;						//使能ECC校验
			for(i=0;i<NAND_ECC_SECTOR_SIZE;i++)				//写入NAND_ECC_SECTOR_SIZE个数据
			{
				*(vu8*)NAND_ADDRESS=*(vu8*)pBuffer++;
			}
			while(!(FMC_Bank3_R->SR&(1<<6)));				//等待FIFO空
			nand_attr.ecc_hdbuf[res+eccstart]=FMC_Bank3_R->ECCR;	//读取硬件计算后的ECC值
  			FMC_Bank3_R->PCR&=~(1<<6);						//禁止ECC校验
		}
		i=nand_attr.page_mainsize+0X10+eccstart*4;			//计算写入ECC的spare区地址
		nand_delay(30);//等待
		*(vu8*)(NAND_ADDRESS|NAND_CMD)=0X85;				//随机写指令
		//发送地址
		*(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)i;
		*(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(i>>8);
		nand_delay(30);//等待tADL
		pBuffer=(u8*)&nand_attr.ecc_hdbuf[eccstart];
		for(i=0;i<eccnum;i++)					//写入ECC
		{
			for(res=0;res<4;res++)
			{
				*(vu8*)NAND_ADDRESS=*(vu8*)pBuffer++;
			}
		}
	}
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_WRITE_TURE1;
    if(wait_ready()!=NSTA_READY)return NSTA_ERROR;//失败
    return 0;//成功
}

//在NAND一页中的指定地址开始,写入指定长度的恒定数字
//PageNum:要写入的页地址,范围:0~(block_pagenum*block_totalnum-1)
//ColNum:要写入的列开始地址(也就是页内地址),范围:0~(page_totalsize-1)
//cval:要写入的指定常数
//NumByteToWrite:要写入的字数(以4字节为单位)
//返回值:0,成功
//    其他,错误代码
u8 nand_page_set_value(u32 PageNum,u16 ColNum,u32 cval,u16 NumByteToWrite)
{
    u16 i=0;
	*(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_WRITE0;
    //发送地址
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)ColNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(ColNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)PageNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(PageNum>>16);
	nand_delay(30);//等待tADL
	for(i=0;i<NumByteToWrite;i++)		//写入数据,每次写4字节
	{
		*(vu32*)NAND_ADDRESS=cval;
	}
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_WRITE_TURE1;
    if(wait_ready()!=NSTA_READY)return NSTA_ERROR;//失败
    return 0;//成功
}



//读取spare区中的数据
//PageNum:要写入的页地址,范围:0~(block_pagenum*block_totalnum-1)
//ColNum:要写入的spare区地址(spare区中哪个地址),范围:0~(page_sparesize-1)
//pBuffer:接收数据缓冲区
//NumByteToRead:要读取的字节数(不大于page_sparesize)
//返回值:0,成功
//    其他,错误代码
u8 nand_read_spare(u32 PageNum,u16 ColNum,u8 *pBuffer,u16 NumByteToRead)
{
    u8 temp=0;
    u8 remainbyte=0;
    remainbyte=nand_attr.page_sparesize-ColNum;
    if(NumByteToRead>remainbyte) NumByteToRead=remainbyte;  //确保要写入的字节数不大于spare剩余的大小
    temp=nand_read_page(PageNum,ColNum+nand_attr.page_mainsize,pBuffer,NumByteToRead);//读取数据
    return temp;
}

//向spare区中写数据
//PageNum:要写入的页地址,范围:0~(block_pagenum*block_totalnum-1)
//ColNum:要写入的spare区地址(spare区中哪个地址),范围:0~(page_sparesize-1)
//pBuffer:要写入的数据首地址
//NumByteToWrite:要写入的字节数(不大于page_sparesize)
//返回值:0,成功
//    其他,失败
u8 nand_write_spare(u32 PageNum,u16 ColNum,u8 *pBuffer,u16 NumByteToWrite)
{
    u8 temp=0;
    u8 remainbyte=0;
    remainbyte=nand_attr.page_sparesize-ColNum;
    if(NumByteToWrite>remainbyte) NumByteToWrite=remainbyte;  //确保要读取的字节数不大于spare剩余的大小
    temp=nand_write_page(PageNum,ColNum+nand_attr.page_mainsize,pBuffer,NumByteToWrite);//读取
    return temp;
}

//擦除一个块
//BlockNum:要擦除的BLOCK编号,范围:0-(block_totalnum-1)
//返回值:0,擦除成功
//    其他,擦除失败
u8 nand_erase_block(u32 BlockNum)
{
	if(nand_attr.id==MT29F16G08ABABA)BlockNum<<=7;  	//将块地址转换为页地址
    else if(nand_attr.id==MT29F4G08ABADA)BlockNum<<=6;
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_ERASE0;
    //发送块地址
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)BlockNum;
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(BlockNum>>8);
    *(vu8*)(NAND_ADDRESS|NAND_ADDR)=(u8)(BlockNum>>16);
    *(vu8*)(NAND_ADDRESS|NAND_CMD)=NAND_ERASE1;
	if(wait_ready()!=NSTA_READY)return NSTA_ERROR;//失败
    return 0;	//成功
}

//全片擦除NAND FLASH
void nand_erase_chip(void)
{
    u8 status;
    u16 i=0;
    for(i=0;i<nand_attr.block_totalnum;i++) //循环擦除所有的块
    {
        status=nand_erase_block(i);
        if(status)
        {
            LOGE("Erase %d block fail!!，错误码为%d\r\n",i,status);//擦除失败
        }
    }
}



nand_attr_t* nand_get_attr(void)
{
    return &nand_attr;
}




