#ifndef __FS_Hx__
#define __FS_Hx__

#include "types.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define FS_NAME_MAX    100


/* Definitions of physical drive number for each drive */
typedef enum {
    DEV_SDMMC=0,      //MMC/SD
    DEV_UDISK,	      //USB DISK
    DEV_SFLASH,       //SPI FLASH
    DEV_PFLASH,       //PARALLEL PORT
    
    DEV_MAX
}FS_DEV;

typedef enum {
    FS_FATFS=0,
    FS_YAFFS,
    FS_STDFS,
    
    FS_MAX
}FS_TYPE;

typedef enum {
    FS_MODE_RW=0,
    FS_MODE_CREATE,
}FS_MODE;


typedef struct {
    int size;
    int isdir;
    char fname[FS_NAME_MAX];
}fs_info_t;

typedef struct {
    U64 total;
    U64 free;
}fs_space_t;



typedef struct {
    int (*init)(FS_DEV dev, U32 start, U32 len);
    int (*mount)(FS_DEV dev, char *path);
    int (*umount)(FS_DEV dev);
    int (*format)(char *path);
    handle_t (*open)(char *path, FS_MODE mode);
    int (*close)(handle_t h);
    int (*read)(handle_t h, void *buf, int len);
    int (*write)(handle_t h, void *buf, int len);
    int (*seek)(handle_t h, int offset);
    int (*sync)(handle_t h);
    int (*truncate)(handle_t h, int len);
    int (*size)(handle_t h);
    int (*remove)(char *path);
    int (*mkdir)(char *path);
    int (*rmdir)(char *path);
    handle_t (*opendir)(char *path);
    int (*closedir)(handle_t h);
    int (*readdir)(handle_t h, fs_info_t *info);
    int (*get_space)(char* path, fs_space_t *sp);
    int (*last_err)(void);
}fs_driver_t;


handle_t fs_init(FS_DEV dev, FS_TYPE tp);
int fs_deinit(handle_t fs);
int fs_mount(handle_t fs, char *path);
int fs_umount(handle_t fs);
handle_t fs_open(handle_t fs, char *path, FS_MODE mode);
int fs_close(handle_t file);
int fs_size(handle_t file);
int fs_read(handle_t file, void *buf, int buflen);
int fs_write(handle_t file, void *buf, int buflen, int sync);
int fs_append(handle_t file, void *buf, int buflen, int sync);
int fs_remove(handle_t file, char *path);
int fs_sync(handle_t file);
int fs_seek(handle_t file, int offset);
int fs_get_space(handle_t fs, fs_space_t *sp);
int fs_scan(handle_t fs, char *path);
int fs_save(handle_t fs, char *path, void *buf, int len);
int fs_print(handle_t fs, char *path, int str_print);

int fs_test(void);

#ifdef __cplusplus
}
#endif

#endif

