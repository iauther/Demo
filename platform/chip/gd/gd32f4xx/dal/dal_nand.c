#include "platform.h"
#include "dal_nand.h"
#include "dal_delay.h"
#include "log.h"

//ecc校验、坏块判断及标定待完善
//https://blog.csdn.net/sudaroot/article/details/99640553


int dal_nand_init(void)
{
    
    return 0;
}




int dal_nand_read_page(u32 page, u8 *data, int data_len, u8 *oob, int oob_len)
{
    U32 ecc[2];
    
#if 0
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
#else

#endif
    
    
    return 0;
}



int dal_nand_write_page(u32 page, u8 *data, int data_len, u8 *oob, int oob_len)
{
    U32 ecc[2];
    
#if 0
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
#else

#endif


    return 0;
}



int dal_nand_erase_block(u32 block)
{
    int s;
    
#if 0
    HAL_StatusTypeDef st;
    NAND_AddressTypeDef addr;
    
    addr.Block = block%NandParam.blocks_per_plane;
    addr.Page  = 0;
    addr.Plane = block/NandParam.blocks_per_plane;
    
    st = HAL_NAND_Erase_Block(&NAND_Handler, &addr);
    if(wait_ready() !=NSTA_READY) {
        return -1;
    }
#else

#endif

    return 0;
}



dal_nand_para_t *dal_nand_get_para(void)
{
    return 0;
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

