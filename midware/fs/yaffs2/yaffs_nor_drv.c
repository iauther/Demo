/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file system.
 *
 * Copyright (C) 2002-2018 Aleph One Ltd.
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * This is an interface module for handling NOR in yaffs1 mode.
 */

/* This code is intended to be used with "regular" NOR that is bit-modifyable.
 *
 * Each "chunk" is a contguous area of 512 + 16 bytes.
 * This makes 248 such chunks with some space left over where a format markerr
 * is stored.
 */

#include "yaffs_nor_drv.h"
#include "yportenv.h"
#include "yaffs_trace.h"
#include "yaffs_flashif.h"
#include "yaffs_guts.h"
#include <string.h>


/* Tunable data */
#define DATA_BYTES_PER_CHUNK	512
#define SPARE_BYTES_PER_CHUNK	16
#define BLOCK_SIZE_IN_BYTES 	(128*1024)
#define BYTES_IN_DEVICE		    (4*1024*1024)

#define BYTES_PER_CHUNK		    (DATA_BYTES_PER_CHUNK + SPARE_BYTES_PER_CHUNK)
#define SPARE_AREA_OFFSET	    DATA_BYTES_PER_CHUNK
#define CHUNKS_PER_BLOCK        (BLOCK_SIZE_IN_BYTES/BYTES_PER_CHUNK)

#define BLOCKS_IN_DEVICE	    (BYTES_IN_DEVICE/BLOCK_SIZE_IN_BYTES)

#define YNOR_PREMARKER          (0xF6)
#define YNOR_POSTMARKER         (0xF0)

#define FORMAT_OFFSET		    (CHUNKS_PER_BLOCK * BYTES_PER_CHUNK)
#define FORMAT_VALUE		    0x1234


/* Compile this to hook up to read hardware */
#include "sflash.h"
#define nor_drv_Init()                      do{} while(0)
#define nor_drv_Deinit()                    do {} while(0)
#define nor_drv_Write32(addr,buf,nwords)    {}//Y_FlashWrite(addr,buf,nwords)
#define nor_drv_Read32(addr,buf,nwords)     {}//Y_FlashRead(addr,buf,nwords)
#define nor_drv_Erase(addr)                 {}//Y_FlashErase(addr,BLOCK_SIZE_IN_BYTES)
#define DEVICE_BASE                         (32 * 1024 * 1024)

static int nor_drv_EraseBlock(struct yaffs_dev *dev, int blockNumber);

static u32 *Block2Addr(struct yaffs_dev *dev, int blockNumber)
{
	u8 *addr;

	dev=dev;

	addr = (u8*)DEVICE_BASE;
	addr += blockNumber * BLOCK_SIZE_IN_BYTES;

	return (u32 *) addr;
}

static u32 *Block2FormatAddr(struct yaffs_dev *dev, int blockNumber)
{
	u8 *addr;

	addr = (u8*) Block2Addr(dev,blockNumber);
	addr += FORMAT_OFFSET;

	return (u32 *)addr;
}

static u32 *Chunk2DataAddr(struct yaffs_dev *dev, int chunk_id)
{
	unsigned block;
	unsigned chunkInBlock;
	u8 *addr;

	block = chunk_id/dev->param.chunks_per_block;
	chunkInBlock = chunk_id % dev->param.chunks_per_block;

	addr = (u8*) Block2Addr(dev,block);
	addr += chunkInBlock * BYTES_PER_CHUNK;

	return (u32 *)addr;
}

static u32 *Chunk2SpareAddr(struct yaffs_dev *dev, int chunk_id)
{
	u8 *addr;

	addr = (u8*) Chunk2DataAddr(dev, chunk_id);
	addr += SPARE_AREA_OFFSET;
	return (u32 *)addr;
}


static void nor_drv_AndBytes(u8*target, const u8 *src, int nbytes)
{
	while(nbytes > 0){
		*target &= *src;
		target++;
		src++;
		nbytes--;
	}
}

static int nor_drv_WriteChunk(struct yaffs_dev *dev,int nand_chunk,
				    const u8 *data, int data_len,
				    const u8 *oob, int oob_len)
{
	u32 *dataAddr = Chunk2DataAddr(dev,nand_chunk);
	u32 *spareAddr = Chunk2SpareAddr(dev,nand_chunk);

	struct yaffs_spare *spare = (struct yaffs_spare *)oob;
	struct yaffs_spare tmpSpare;

	(void) oob_len;

	/* We should only be getting called for one of 3 reasons:
         * Writing a chunk: data and spare will not be NULL
         * Writing a deletion marker: data will be NULL, spare not NULL
         * Writing a bad block marker: data will be NULL, spare not NULL
         */

	if(sizeof(struct yaffs_spare) != SPARE_BYTES_PER_CHUNK)
		BUG();

	if(data && oob) {
		if(spare->page_status != 0xff)
			BUG();
		/* Write a pre-marker */
		memset(&tmpSpare,0xff,sizeof(tmpSpare));
		tmpSpare.page_status = YNOR_PREMARKER;
		nor_drv_Write32(spareAddr,(u32 *)&tmpSpare,sizeof(struct yaffs_spare)/sizeof(u32));

		/* Write the data */
		nor_drv_Write32(dataAddr,(u32 *)data, data_len/ sizeof(u32));

		memcpy(&tmpSpare,spare,sizeof(struct yaffs_spare));

		/* Write the real tags, but override the premarker*/
		tmpSpare.page_status = YNOR_PREMARKER;
		nor_drv_Write32(spareAddr,(u32 *)&tmpSpare,sizeof(struct yaffs_spare)/sizeof(u32));

		/* Write a post-marker */
		tmpSpare.page_status = YNOR_POSTMARKER;
		nor_drv_Write32(spareAddr,(u32 *)&tmpSpare,sizeof(tmpSpare)/sizeof(u32));

	} else if(spare){
		/* This has to be a read-modify-write operation to handle NOR-ness */

		nor_drv_Read32(spareAddr,(u32 *)&tmpSpare,sizeof(struct yaffs_spare)/sizeof(u32));

		nor_drv_AndBytes((u8 *)&tmpSpare,(u8 *)spare,sizeof(struct yaffs_spare));

		nor_drv_Write32(spareAddr,(u32 *)&tmpSpare,sizeof(struct yaffs_spare)/sizeof(u32));
	} else {
//		BUG();
	}

	return YAFFS_OK;
}

static int nor_drv_ReadChunk(struct yaffs_dev *dev,int nand_chunk,
					u8 *data, int data_len,
					u8 *oob, int oob_len,
					enum yaffs_ecc_result *ecc_result)
{
	struct yaffs_spare *spare = (struct yaffs_spare *)oob;

	u32 *dataAddr = Chunk2DataAddr(dev,nand_chunk);
	u32 *spareAddr = Chunk2SpareAddr(dev,nand_chunk);

	(void) data_len;

	if (data) {
		nor_drv_Read32(dataAddr,(u32 *)data,dev->param.total_bytes_per_chunk / sizeof(u32));
	}

	if (oob) {
		nor_drv_Read32(spareAddr,(u32 *)spare, oob_len/ sizeof(u32));

		/* If the page status is YNOR_POSTMARKER then it was written properly
           so change that to 0xFF so that the rest of yaffs is happy. */
		if(spare->page_status == YNOR_POSTMARKER)
			spare->page_status = 0xff;
		else if(spare->page_status != 0xff &&
			(spare->page_status | YNOR_PREMARKER) != 0xff)
			spare->page_status = YNOR_PREMARKER;
	}

	if(ecc_result)
		*ecc_result = YAFFS_ECC_RESULT_NO_ERROR;

	return YAFFS_OK;

}


static int nor_drv_FormatBlock(struct yaffs_dev *dev, int blockNumber)
{
	u32 *blockAddr = Block2Addr(dev,blockNumber);
	u32 *formatAddr = Block2FormatAddr(dev,blockNumber);
	u32 formatValue = FORMAT_VALUE;

	nor_drv_Erase(blockAddr);
	nor_drv_Write32(formatAddr,&formatValue,1);

	return YAFFS_OK;
}

static int nor_drv_UnformatBlock(struct yaffs_dev *dev, int blockNumber)
{
	u32 *formatAddr = Block2FormatAddr(dev,blockNumber);
	u32 formatValue = 0;

	nor_drv_Write32(formatAddr,&formatValue,1);

	return YAFFS_OK;
}

static int nor_drv_IsBlockFormatted(struct yaffs_dev *dev, int blockNumber)
{
	u32 *formatAddr = Block2FormatAddr(dev,blockNumber);
	u32 formatValue;


	nor_drv_Read32(formatAddr,&formatValue,1);

	return (formatValue == FORMAT_VALUE);
}

static int nor_drv_EraseBlock(struct yaffs_dev *dev, int blockNumber)
{

	if(blockNumber < 0 || blockNumber >= BLOCKS_IN_DEVICE) {
		yaffs_trace(YAFFS_TRACE_ALWAYS,
			"Attempt to erase non-existant block %d\n",
			blockNumber);
		return YAFFS_FAIL;
	}
	else {
		nor_drv_UnformatBlock(dev,blockNumber);
		nor_drv_FormatBlock(dev,blockNumber);
		return YAFFS_OK;
	}

}

static int nor_drv_Initialise(struct yaffs_dev *dev)
{
	u32 i;

	nor_drv_Init();
	/* Go through the blocks formatting them if they are not formatted */
	for(i = dev->param.start_block; i <= dev->param.end_block; i++){
		if(!nor_drv_IsBlockFormatted(dev,i)){
			nor_drv_FormatBlock(dev,i);
		}
	}
	return YAFFS_OK;
}

static int nor_drv_Deinitialise(struct yaffs_dev *dev)
{
	dev=dev;

	nor_drv_Deinit();

	return YAFFS_OK;
}


struct yaffs_dev *yaffs_nor_init(u32 startAddr, u32 length)
{
	struct yaffs_dev *dev = malloc(sizeof(struct yaffs_dev));
	struct yaffs_param *param;
	struct yaffs_driver *drv;

	if(!dev) {
		return NULL;
	}

	param = &dev->param;
	drv = &dev->drv;

	memset(dev, 0, sizeof(*dev));

	param->name = NULL; //name_copy;

	param->total_bytes_per_chunk = DATA_BYTES_PER_CHUNK;
	param->chunks_per_block = CHUNKS_PER_BLOCK;
	param->n_reserved_blocks = 2;
	param->start_block = 0; // Can use block 0
	param->end_block = BLOCKS_IN_DEVICE - 1; // Last block
	param->use_nand_ecc = 0; // use YAFFS's ECC
	param->disable_soft_del = 1;

	drv->drv_write_chunk_fn = nor_drv_WriteChunk;
	drv->drv_read_chunk_fn = nor_drv_ReadChunk;
	drv->drv_erase_fn = nor_drv_EraseBlock;
	drv->drv_initialise_fn = nor_drv_Initialise;
	drv->drv_deinitialise_fn = nor_drv_Deinitialise;

	param->n_caches = 10;
	param->disable_soft_del = 1;

//	dev->driver_context = (void *) nor_sim;

	yaffs_add_device(dev);

	return dev;
}
