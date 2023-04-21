#ifndef __FS_Hx__
#define __FS_Hx__


#ifdef __cplusplus
 extern "C" {
#endif

#ifdef _WIN32
    #include<unistd.h>
    #define fhand_t     FILE*
    #define FNULL       NULL

    #define FINVALID(fd) (fd==FNULL)
    typedef struct stat stat_t;

    #define fileno(fd)  fd
    #define _finit       finit  
    #define _fmount      fmount 
    #define _format      format
    #define _fopoen      fopen
    #define _fread       fread  
    #define _fwrite      fwrite 
    #define _fseek       fseek  
    #define _ftruncate   ftruncate
    #define _fstat       fstat  
    #define _fclose      fclose 
    #define _fflush      fflush 
    #define _fremvoe     remove
    
    #define _fmkdir      mkdir
    #define _frmdir      rmdir
    
    #define _last_err    GetLastError
#else
    #include "yaffsfs.h"
    #include "yaffs_nand_drv.h"
    //#include "fatfs.h"
    //#include "littlefs.h"
    #define fhand_t     int
    #define FNULL       (-1)

    #define FINVALID(fd) (fd<0)
    typedef struct yaffs_stat stat_t;

    #define _fileno(fd)  fd
    #define _finit       yaffs_nand_init
    #define _fmount      yaffs_mount
    #define _format      yaffs_format
    #define _fopen       yaffs_open  
    #define _fread       yaffs_read
    #define _fwrite      yaffs_write
    #define _fseek       yaffs_lseek
    #define _ftruncate   yaffs_ftruncate
    #define _fstat       yaffs_fstat
    #define _fclose      yaffs_close
    #define _fflush      yaffs_flush
    #define _fremvoe     yaffs_unlink
    
    #define _fmkdir      yaffs_mkdir
    #define _frmdir      yaffs_rmdir
    
    #define _last_err    yaffsfs_GetLastError
#endif

int fs_init(void);
int fs_read(int id, void *buf, int buflen);
int fs_write(int id, void *buf, int buflen);
int fs_remove(int id);
int fs_sync(int id);
int fs_test(void);

#ifdef __cplusplus
}
#endif
#endif /*__fatfs_H */
