#include "yaffs_nand_drv.h"
#include "dal/nand.h"
#include "yportenv.h"


static int drv_initialise_fn(struct yaffs_dev *dev)
{
    int r = YAFFS_OK;
    
	return r;
}
static int drv_deinitialise_fn(struct yaffs_dev *dev)
{
	return YAFFS_OK;
}
static int drv_mark_bad_fn(struct yaffs_dev *dev, int block_no)
{
    int r = YAFFS_OK;
#if 0
	if(NandFlash_MarkBadBlock(block_no) == TRUE) {
		return YAFFS_OK;
	}
#else
    
#endif
    
	
	return r;
}
static int drv_check_bad_fn(struct yaffs_dev *dev, int block_no)
{
	int r = YAFFS_OK;
#if 0
	if(NandFlash_IsBadBlock(block_no) == TRUE) {//bad block
		res = YAFFS_FAIL;
	}
#else
    
#endif
	
	return r;
}
static int drv_erase_fn(struct yaffs_dev *dev, int block_no)
{
    int r = YAFFS_OK;
    
#if 0
	if(NandFlash_BlockErase(block_no) == TRUE) {
		return YAFFS_OK;
	}
#endif
    
    nand_erase_block(block_no);
    
	return r;
}
static int drv_read_chunk_fn (struct yaffs_dev *dev, int nand_chunk, 
	u8 *data, int data_len, u8 *oob, int oob_len, enum yaffs_ecc_result *ecc_result)
{
#if 0
	uint32_t blockId = nand_chunk / NANDFLASH_PAGE_PER_BLOCK;
	uint32_t pageOffset = nand_chunk % NANDFLASH_PAGE_PER_BLOCK;
	uint8_t i;
	uint8_t test_ecc[ECC_RESULT_LEN];
	uint8_t *read_ecc = NULL;
	int result = 0;
	uint8_t fixed = 0;

	if(NandFlash_IsBadBlock(blockId) == TRUE) {
		return YAFFS_FAIL;
	}
	
	if(BAD_BLOCK_INFO_BITS + oob_len + ECC_TOTAL_LEN >= NANDFLASH_SPARE_SIZE) {
		return YAFFS_FAIL;
	}
	
	memset(g_tmp_page, 0xFF, sizeof(g_tmp_page));
	
	if(NandFlash_PageRead(blockId, pageOffset, g_tmp_page) == FALSE) {
		*ecc_result = YAFFS_ECC_RESULT_UNKNOWN;
		return YAFFS_FAIL;
	}
	
	if(data && data_len > 0) {
		memcpy(data, g_tmp_page, data_len);
	}
	
	if(oob && oob_len > 0) {
		memcpy(oob, g_tmp_page + NANDFLASH_RW_PAGE_SIZE + BAD_BLOCK_INFO_BITS, oob_len);
	}

	
	for(i=0; i<ECC_CALC_COUNT; i++) {
		read_ecc = g_tmp_page + NANDFLASH_RW_PAGE_SIZE + BAD_BLOCK_INFO_BITS + oob_len + i * ECC_RESULT_LEN;

		yaffs_ecc_calc(g_tmp_page + i * ECC_PER_CALC_LEN, test_ecc);
		result = yaffs_ecc_correct(data, read_ecc, test_ecc);
		if(result < 0) { //Unrecoverable error
			*ecc_result = YAFFS_ECC_RESULT_UNFIXED;
			return YAFFS_OK;
		} else if(result == 1) { //Corrected the error
			fixed = 1;
		}
	}

	if(fixed == 1) {
		*ecc_result = YAFFS_ECC_RESULT_FIXED;
	} else {
		*ecc_result = YAFFS_ECC_RESULT_NO_ERROR;
	}
#else
    int r;
    
    r = nand_read_page(nand_chunk, (u8*)data, data_len, oob, oob_len);
    if(r==NSTA_ECC1BITERR) {
        *ecc_result = YAFFS_ECC_RESULT_FIXED;
    }
    else if(r==NSTA_ECC2BITERR) {
        *ecc_result = YAFFS_ECC_RESULT_UNFIXED;
    }
    else if(r==NSTA_ERROR) {
        *ecc_result = YAFFS_ECC_RESULT_UNKNOWN;
    }
    else {
        *ecc_result = YAFFS_ECC_RESULT_NO_ERROR;
    }
    
#endif

	return YAFFS_OK;	
}
static int drv_write_chunk_fn (struct yaffs_dev *dev, int nand_chunk, 
	const u8 *data, int data_len, const u8 *oob, int oob_len)
{
#if 0
	uint32_t blockId = nand_chunk / NANDFLASH_PAGE_PER_BLOCK;
	uint32_t pageOffset = nand_chunk % NANDFLASH_PAGE_PER_BLOCK;
	uint8_t i;
	uint8_t *ecc_pos = NULL;

	if(NandFlash_IsBadBlock(blockId) == TRUE) {
		return YAFFS_FAIL;
	}
	
	if(BAD_BLOCK_INFO_BITS + oob_len + ECC_TOTAL_LEN >= NANDFLASH_SPARE_SIZE) {
		return YAFFS_FAIL;
	}
	
	memset(g_tmp_page, 0xFF, sizeof(g_tmp_page));
	
	if(data && data_len > 0)
		memcpy(g_tmp_page, data, data_len);
	
	if(oob && oob_len > 0)
    memcpy(g_tmp_page + NANDFLASH_RW_PAGE_SIZE + BAD_BLOCK_INFO_BITS, oob, oob_len);
	
	for(i=0; i<ECC_CALC_COUNT; i++) {
		ecc_pos = g_tmp_page + NANDFLASH_RW_PAGE_SIZE + BAD_BLOCK_INFO_BITS + oob_len + i * ECC_RESULT_LEN;
		yaffs_ecc_calc(g_tmp_page + i * ECC_PER_CALC_LEN, ecc_pos);
	}
	
	if (NandFlash_PageProgram( blockId, pageOffset, g_tmp_page, TRUE) == FALSE)
		return YAFFS_FAIL;
#else
    int r;
    
    r = nand_write_page(nand_chunk, (u8*)data, data_len, (u8*)oob, oob_len);
#endif

	return YAFFS_OK;
}

struct yaffs_dev *yaffs_nand_init(const YCHAR *dev_name,
		u32 dev_id, u32 startAddr, u32 length)
{
	struct yaffs_dev *dev = NULL;
	struct yaffs_driver *drv = NULL;
	struct yaffs_param *param = NULL;
    nand_para_t *para=nand_get_para();
    u32 total_blocks,head_blocks,block_len;
    u32 blocks,start_block,end_block;
    
    nand_init();
    
	dev = yaffsfs_malloc(sizeof(struct yaffs_dev));
	if(dev == NULL) {
		return dev;
	}
    
    block_len = para->bytes_per_page*para->pages_per_block;
    total_blocks = para->blocks_per_plane*para->planes_per_chip;
    head_blocks = startAddr/block_len;
    
    blocks = length/block_len;
    start_block = head_blocks;
    end_block   = start_block+blocks-1;
	
	if((end_block == 0) || (end_block >= (start_block + blocks)))
        end_block = (start_block + blocks) - 1;
    
	memset(dev, 0, sizeof(struct yaffs_dev));
	param = &(dev->param);
	drv = &(dev->drv);
	
	param->name = dev_name;
	param->total_bytes_per_chunk = para->bytes_per_page;                                                     //NANDFLASH_RW_PAGE_SIZE;
	param->chunks_per_block = para->pages_per_block*para->bytes_per_page/param->total_bytes_per_chunk;         //NANDFLASH_PAGE_PER_BLOCK;
	param->spare_bytes_per_chunk = para->bytes_per_spare*(param->total_bytes_per_chunk/para->bytes_per_page); //NANDFLASH_SPARE_SIZE;
	param->n_reserved_blocks = 5;
	param->start_block = start_block;
	param->end_block = end_block;
	param->is_yaffs2 = 1;
	param->n_caches = 10;
    param->use_nand_ecc = 1;
    param->inband_tags = 0;
	
	drv->drv_write_chunk_fn = drv_write_chunk_fn;
	drv->drv_read_chunk_fn = drv_read_chunk_fn;
	drv->drv_erase_fn = drv_erase_fn;
	drv->drv_mark_bad_fn = drv_mark_bad_fn;
	drv->drv_check_bad_fn = drv_check_bad_fn;
	drv->drv_initialise_fn = drv_initialise_fn;
	drv->drv_deinitialise_fn = drv_deinitialise_fn;
	
	yaffs_add_device(dev);

	return dev;
}


