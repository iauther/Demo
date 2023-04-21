#ifndef __DAL_NAND_Hx__
#define __DAL_NAND_Hx__

#include "types.h"
	
#define NAND_MAX_PAGE_SIZE			4096		//定义NAND FLASH的最大的PAGE大小（不包括SPARE区），默认4096字节
#define NAND_ECC_SECTOR_SIZE		512			//执行ECC计算的单元大小，默认512字节

//NAND属性结构体
typedef struct
{
    u16 page_totalsize;     	//每页总大小，main区和spare区总和
    u16 page_mainsize;      	//每页的main区大小
    u16 page_sparesize;     	//每页的spare区大小
    u8  block_pagenum;      	//每个块包含的页数量
    u16 plane_blocknum;     	//每个plane包含的块数量
    u16 block_totalnum;     	//总的块数量
    u16 good_blocknum;      	//好块数量    
    u16 valid_blocknum;     	//有效块数量(供文件系统使用的好块数量)
    u32 id;             		//NAND FLASH ID
    u16 *lut;      			   	//LUT表，用作逻辑块-物理块转换
	u32 ecc_hard;				//硬件计算出来的ECC值
	u32 ecc_hdbuf[NAND_MAX_PAGE_SIZE/NAND_ECC_SECTOR_SIZE];//ECC硬件计算值缓冲区  	
	u32 ecc_rdbuf[NAND_MAX_PAGE_SIZE/NAND_ECC_SECTOR_SIZE];//ECC读取的值缓冲区
}nand_attr_t;      

typedef struct
{
    u16 bytes_per_page;
    u16 bytes_per_spare;
    u16 pages_per_block;
    u16 blocks_per_plane;
    u16 planes_per_chip;
}dal_nand_para_t; 



#define NAND_RB  	            HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_6) // NAND Flash的闲/忙引脚

#define NAND_ADDRESS			0X80000000	//nand flash的访问地址,接NCE3,地址为:0X8000 0000
#define NAND_CMD				1<<16		//发送命令
#define NAND_ADDR				1<<17		//发送地址

//NAND FLASH命令
#define NAND_READID         	0X90    	//读ID指令
#define NAND_FEATURE			0XEF    	//设置特性指令
#define NAND_RESET          	0XFF    	//复位NAND
#define NAND_READSTA        	0X70   	 	//读状态
#define NAND_AREA_A         	0X00   
#define NAND_AREA_TRUE1     	0X30  
#define NAND_WRITE0        	 	0X80
#define NAND_WRITE_TURE1    	0X10
#define NAND_ERASE0        	 	0X60
#define NAND_ERASE1         	0XD0
#define NAND_MOVEDATA_CMD0  	0X00
#define NAND_MOVEDATA_CMD1  	0X35
#define NAND_MOVEDATA_CMD2  	0X85
#define NAND_MOVEDATA_CMD3  	0X10

//NAND FLASH状态
#define NSTA_READY       	   	0X40		//nand已经准备好
#define NSTA_ERROR				0X01		//nand错误
#define NSTA_TIMEOUT        	0X02		//超时
#define NSTA_ECC1BITERR       	0X03		//ECC 1bit错误
#define NSTA_ECC2BITERR       	0X04		//ECC 2bit以上错误

//NAND FLASH型号和对应的ID号
#define MT29F4G08ABADA			0XDC909556	//MT29F4G08ABADA
#define MT29F16G08ABABA			0X48002689	//MT29F16G08ABABA

//MPU相关设置
#define NAND_REGION_NUMBER      MPU_REGION_NUMBER4	    //NAND FLASH使用region4
#define NAND_ADDRESS_START      0X80000000              //NAND FLASH区的首地址
#define NAND_REGION_SIZE        MPU_REGION_SIZE_256MB   //NAND FLASH区大小

int dal_nand_init(void);
int dal_nand_read_page(u32 page, u8 *data, int data_len, u8 *oob, int oob_len);
int dal_nand_write_page(u32 page, u8 *data, int data_len, u8 *oob, int oob_len);
int dal_nand_erase_block(u32 block);
dal_nand_para_t *dal_nand_get_para(void);

#endif
