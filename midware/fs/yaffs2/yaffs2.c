#include "yaffsfs.h"
#include "log.h"
#include "fs.h"
#include "cfg.h"


#ifdef USE_YAFFS

typedef struct {
    FS_DEV      dev;
    int         fp;
    
    struct yaffs_dev* xx;
}yaffs2_handle_t;


typedef struct {
    char        mpath[64];      //mount path
    U32         start;
    U32         len;
}yaffs2_info_t;
static yaffs2_info_t *yaffsInfo[DEV_MAX]={NULL};


static int yaffs2_init(FS_DEV dev, U32 start, U32 len)
{
    switch((int)dev) {
        case DEV_MMC:
        case DEV_USB:
        case DEV_PPT:
        yaffs_init(YAFFS_NAND, start, len);
        break;
        
        
        case DEV_SFL:
        yaffs_init(YAFFS_NAND, start, len);
        break;
    }
    
    return 0;
}


static int yaffs2_mount(FS_DEV dev, char *path)
{
    int r;
    yaffs2_info_t *info=NULL;
    
    if(yaffsInfo[dev]) {
        LOGD("device is already mounted!\n");
        return 0;
    }
    
    info = (yaffs2_info_t*)malloc(sizeof(yaffs2_info_t));;
    if(!info) {
        return -1;
    }
    
    r = yaffs_mount(path);
    if(r) {
        free(info);
        return -1;
    }
    
    strcpy(info->mpath, path);
    yaffsInfo[dev] = info;
        
    return 0;
}


static int yaffs2_umount(FS_DEV dev)
{    
    if(!yaffsInfo[dev]) {
        return -1;
    }
    
    yaffs_unmount(yaffsInfo[dev]->mpath);
    free(yaffsInfo[dev]);
    yaffsInfo[dev] = NULL;
    
    return 0;
}


static int yaffs2_format(FS_DEV dev, char *path)
{
    return yaffs_format(path, 1, 1, 1);
}



static handle_t yaffs2_open(char *path, FS_MODE mode)
{
    int i,r=-1,flag;
    char tmp[100];
    yaffs2_info_t *info=NULL;
    yaffs2_handle_t *h=NULL;
    
    if(!path) {
        return NULL;
    }
    
    h = (yaffs2_handle_t*)malloc(sizeof(yaffs2_handle_t));
    if(!h) {
        LOGE("yaffs2_open() malloc %d failed!\n", sizeof(yaffs2_handle_t));
        return NULL;
    }
    
    for(i=0; i<DEV_MAX; i++) {
        if(yaffsInfo[i] && strstr(path, yaffsInfo[i]->mpath)) {
            info = yaffsInfo[i];
        }
    }
    
    if(!info) {
        LOGE("___ device not mount!\n");
        free(h);
        return NULL;
    }
    
    flag = O_RDWR|O_TRUNC;
    if(mode==FS_MODE_CREATE) {
        flag |= O_CREAT;
    }
    h->fp = yaffs_open(path, flag, S_IREAD|S_IWRITE);
    if(r) {
        free(h);
        return NULL;
    }
            
    return h;
}


static int yaffs2_close(handle_t h)
{
    yaffs2_handle_t *hf=(yaffs2_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    yaffs_close(hf->fp);
    free(hf);
    
    return 0;
}


static int yaffs2_read(handle_t h, void *buf, int len)
{
    int r;
    yaffs2_handle_t *hf=(yaffs2_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return yaffs_read(hf->fp, buf, len);
}


static int yaffs2_write(handle_t h, void *buf, int len)
{
    int r;
    yaffs2_handle_t *hf=(yaffs2_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return yaffs_write(hf->fp, buf, len);
}


static int yaffs2_seek(handle_t h, int offset)
{
    yaffs2_handle_t *hf=(yaffs2_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return yaffs_lseek(hf->fp, offset, SEEK_SET);
}


static int yaffs2_sync(handle_t h)
{
    yaffs2_handle_t *hf=(yaffs2_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return yaffs_fsync(hf->fp);
}


static int yaffs2_truncate(handle_t h, int len)
{
    int r;
    yaffs2_handle_t *hf=(yaffs2_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    r = yaffs_ftruncate(hf->fp, len);
    
    return r;
}


static int yaffs2_size(handle_t h)
{
    int r;
    struct yaffs_stat st;
    yaffs2_handle_t *hf=(yaffs2_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    r = yaffs_fstat(hf->fp, &st);
    if(r) {
        return -1;
    }
    
    return st.st_size;
}


static int yaffs2_length(char *path)
{
    int r;
    struct yaffs_stat st;
    yaffs2_handle_t *hf=(yaffs2_handle_t*)h;
    
    if(!path || !length) {
        return -1;
    }
    
    r = yaffs_stat(path, &st);
    if(r) {
        return -1;
    }
    return st.st_size;
}


static int yaffs2_exist(char *path)
{
    int r;
    struct yaffs_stat st;
    yaffs2_handle_t *hf=(yaffs2_handle_t*)h;
    
    if(!path || !length) {
        return -1;
    }
    
    r = yaffs_stat(path, &st);
    if(r==0) {
        return 1;
    }
    return 0;
}



static int yaffs2_remove(char *path)
{
    return yaffs_unlink(path);
}


static int yaffs2_mkdir(char *path)
{
    return yaffs_mkdir(path, S_IREAD|S_IWRITE);
}


static int yaffs2_rmdir(char *path)
{
    return yaffs_rmdir(path);
}


static handle_t yaffs2_opendir(char *path)
{
    return yaffs_opendir(path);
}


static int yaffs2_closedir(handle_t h)
{
    struct yaffs_dirent *dir;
    dir = yaffs_closedir((yaffs_DIR*)h);
}


static int yaffs2_readdir(handle_t h, fs_info_t *info)
{
    int slen;
    struct yaffs_dirent *de;
    struct yaffs_stat st;
    yaffs_DIR *dir=£¨yaffs_DIR*£©h;
    
    de = yaffs_readdir(dir);
    if(de) {
        //yaffs_lstat(str ,&st);
        if((st.st_mode&S_IFMT)==S_IFDIR) {
            info->isdir = 1;
        }
        else {
            info->isdir = 0;
        }
        
        slen = strlen(de->d_name);
        if(slen>=FS_NAME_MAX-1) {
            memcpy(info->fname, de->d_name, FS_NAME_MAX-1);
            info->fname[FS_NAME_MAX-1] = 0;
        }
        else {
            strcpy(info->fname, de->d_name);
        }
    }
}


static int yaffs2_get_space(char *path, fs_space_t *sp)
{
    yaffs2_info_t *info;


    if(!path || !sp) {
        return -1;
    }

    sp->free = yaffs_freespace(path);
    sp->total = yaffs_totalspace(path);
    
    return 0;
}


static int yaffs2_last_err(void)
{
    return yaffsfs_GetLastError();
}



fs_driver_t yaffs2_driver={
    
    yaffs2_init,
    yaffs2_mount,
    yaffs2_umount,
    yaffs2_format,
    yaffs2_open,
    yaffs2_close,
    yaffs2_read,
    yaffs2_write,
    yaffs2_seek,
    yaffs2_sync,
    yaffs2_truncate,
    yaffs2_size,
    yaffs2_length,
    yaffs2_exist,
    yaffs2_remove,
    yaffs2_mkdir,
    yaffs2_rmdir,
    yaffs2_opendir,
    yaffs2_closedir,
    yaffs2_readdir,
    yaffs2_get_space,
    yaffs2_last_err,
};
#endif


