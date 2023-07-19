#ifndef __DAL_NAND_Hx__
#define __DAL_NAND_Hx__

#include "types.h"
	
typedef struct
{
    u16 bytes_per_page;
    u16 bytes_per_spare;
    u16 pages_per_block;
    u16 blocks_per_plane;
    u16 planes_per_chip;
}dal_nand_para_t; 


int dal_nand_init(void);
int dal_nand_read_page(u32 page, u8 *data, int data_len, u8 *oob, int oob_len);
int dal_nand_write_page(u32 page, u8 *data, int data_len, u8 *oob, int oob_len);
int dal_nand_erase_block(u32 block);
dal_nand_para_t *dal_nand_get_para(void);

#endif
