/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "dal_sd.h"
#include "sflash.h"
#include "fs.h"


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat=STA_NOINIT;
	int result=0;

	switch (pdrv) {
        case DEV_SDMMC:
        {
            return RES_OK;
        }
        break;

        case DEV_UDISK :
        {
        }
        break;
        
        case DEV_SFLASH :
        {
            return RES_OK;
        }
        break;
        
        case DEV_PFLASH :
        {
        }
        break;
	}
    
	return stat;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
    int result;
	DSTATUS stat=STA_NOINIT;

	switch (pdrv) {
        case DEV_SDMMC :
		{
            result=dal_sd_init();
            if(result==0) {
                return RES_OK;
            }
        }
        break;

        case DEV_UDISK :
		{
            //
        }
        break;
        
        case DEV_SFLASH :
        {
            return RES_OK;
        }
        break;
        
        case DEV_PFLASH :
        {
        }
        break;
	}
    
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res=RES_PARERR;
	int result;

	switch (pdrv) {
        case DEV_SDMMC :
        {
            result = dal_sd_read((U8*)buff, sector, count);
            if(result) {
                return RES_ERROR;
            }
        }
        break;

        case DEV_UDISK :
        {
        }
        break;
        
        case DEV_SFLASH :
        {
            U32 xsec=512+sector;
            U32 xaddr=xsec*4096;
            
            sflash_read(xaddr, buff, count);
        }
        break;
        
        case DEV_PFLASH :
        {
        }
        break;
    }

	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res=RES_OK;
	int result;

	switch (pdrv) {
        case DEV_SDMMC :
        {
            result = dal_sd_write((U8*)buff, sector, count);
            if(result) {
                return RES_PARERR;
            }
        }
        break;

        case DEV_UDISK :
        {
        }
        break;
        
        case DEV_SFLASH :
        {
            U32 xsec=512+sector;
            U32 xaddr=xsec*4096;
            
            sflash_erase_sector(xsec);
            sflash_write(xaddr, (void*)buff, count, 0);
        }
        break;
        
        case DEV_PFLASH :
        {
        }
        break;
    }

	return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res=RES_PARERR;;
	int result;

	switch (pdrv) {
        case DEV_SDMMC :
		{
            dal_sdcard_info_t info;
            
            dal_sd_get_info(&info);
            switch (cmd) {
                case CTRL_SYNC :
                break;

                case GET_SECTOR_COUNT :
                {
                    *(DWORD*)buff = info.capacity/info.secsize;
                }
                break;

                case GET_SECTOR_SIZE:
                {
                    *(DWORD*)buff = info.capacity/info.secsize;
                }
                break;
                
                case GET_BLOCK_SIZE :
                {
                    *(WORD*)buff = info.blksize;
                }
                break;

                default:
                return RES_PARERR;
            }
        }
        break;

        case DEV_UDISK :
		{
        }
        break;
        
        case DEV_SFLASH :
        {
            switch (cmd) {
                case CTRL_SYNC :
                break;

                case GET_SECTOR_COUNT :
                {
                    *(DWORD*)buff = 4096-512;
                }
                break;

                case GET_SECTOR_SIZE:
                {
                    *(DWORD*)buff = 4096;
                }
                break;
                
                case GET_BLOCK_SIZE :
                {
                    *(WORD*)buff = 1;
                }
                break;

                default:
                return RES_PARERR;
            }
        }
        break;
        
        case DEV_PFLASH :
        {
        }
        break;
	}

	return RES_OK;
}

