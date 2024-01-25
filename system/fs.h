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
    int (*format)(FS_DEV dev, char *path);
    handle_t (*open)(char *path, FS_MODE mode);
    int (*close)(handle_t h);
    int (*read)(handle_t h, void *buf, int len);
    int (*write)(handle_t h, void *buf, int len);
    int (*seek)(handle_t h, int offset);
    int (*sync)(handle_t h);
    int (*truncate)(handle_t h, int len);
    int (*size)(handle_t h);
    int (*length)(char *path);
    int (*exist)(char *path);
    int (*remove)(char *path);
    int (*mkdir)(char *path);
    int (*rmdir)(char *path);
    handle_t (*opendir)(char *path);
    int (*closedir)(handle_t h);
    int (*readdir)(handle_t h, fs_info_t *info);
    int (*get_space)(char* path, fs_space_t *sp);
    int (*last_err)(void);
}fs_driver_t;


int fs_init(void);
int fs_deinit(void);
handle_t fs_open(char *path, FS_MODE mode);
handle_t fs_openx(U8 id, FS_MODE mode);
int fs_close(handle_t file);
int fs_size(handle_t file);
int fs_load(char *path, void *buf, int buflen);
int fs_read(handle_t file, void *buf, int buflen);
int fs_write(handle_t file, void *buf, int buflen, int sync);
int fs_append(handle_t file, void *buf, int buflen, int sync);
int fs_remove(char *path);
int fs_sync(handle_t file);
int fs_seek(handle_t file, int offset);
int fs_scan(char *path, handle_t l, int *nfiles);
int fs_length(char *path);
int fs_exist(char *path);

int fs_lengthx(int id);
int fs_readx(int id, void *buf, int len);
int fs_savex(int id, void *buf, int len);

int fs_get_space(char *path, fs_space_t *sp);
int fs_save(char *path, void *buf, int len);
int fs_print(char *path, int str_print);
int fs_test(void);

#ifdef __cplusplus
}
#endif

#endif

