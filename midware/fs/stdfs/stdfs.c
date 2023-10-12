#include <stdio.h>
//#include <unistd.h>
//#include <direct.h>
#include "log.h"
#include "fs.h"
#include "cfg.h"


#ifdef USE_STDFS

typedef struct {
    FS_DEV      dev;
    FILE        *fp;
}stdfs_handle_t;


typedef struct {
    char        mpath[64];      //mount path
    U32         start;
    U32         len;
}stdfs_info_t;
static stdfs_info_t *stdfsInfo[DEV_MAX]={NULL};


static int stdfs_init(FS_DEV dev, U32 start, U32 len)
{
    return 0;
}


static int stdfs_mount(FS_DEV dev, char *path)
{
    int r;
    stdfs_info_t *info=NULL;
    
    if(stdfsInfo[dev]) {
        LOGD("device is already mounted!\n");
        return 0;
    }
    
    info = (stdfs_info_t*)malloc(sizeof(stdfs_info_t));;
    if(!info) {
        return -1;
    }
    
    r = 0;//mount(path);
    if(r) {
        free(info);
        return -1;
    }
    
    strcpy(info->mpath, path);
    
    stdfsInfo[dev] = info;
        
    return 0;
}


static int stdfs_umount(FS_DEV dev)
{    
    if(!stdfsInfo[dev]) {
        return -1;
    }
    
    //umount(stdfsInfo[dev]->mpath);
    free(stdfsInfo[dev]);
    stdfsInfo[dev] = NULL;
    
    return 0;
}


static int stdfs_format(FS_DEV dev, char *path)
{
    return 0;//fformat(path, 1, 1, 1);
}



static handle_t stdfs_open(char *path, FS_MODE mode)
{
    int i,r=-1;
    char tmp[100];
    stdfs_info_t *info=NULL;
    stdfs_handle_t *h=NULL;
    
    if(!path) {
        return NULL;
    }
    
    h = (stdfs_handle_t*)malloc(sizeof(stdfs_handle_t));
    if(!h) {
        LOGE("stdfs_open() malloc %d failed!\n", sizeof(stdfs_handle_t));
        return NULL;
    }
    
    for(i=0; i<DEV_MAX; i++) {
        if(stdfsInfo[i] && strstr(path, stdfsInfo[i]->mpath)) {
            info = stdfsInfo[i];
        }
    }
    
    if(!info) {
        LOGE("___ device not mount!\n");
        free(h);
        return NULL;
    }
    
    h->fp = fopen(path, (mode==FS_MODE_CREATE)?"w":"r+");
    if(!h->fp) {
        free(h);
        return NULL;
    }
            
    return h;
}


static int stdfs_close(handle_t h)
{
    stdfs_handle_t *hf=(stdfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    fclose(hf->fp);
    free(hf);
    
    return 0;
}


static int stdfs_read(handle_t h, void *buf, int len)
{
    int r;
    stdfs_handle_t *hf=(stdfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return fread(buf, 1, len, hf->fp);
}


static int stdfs_write(handle_t h, void *buf, int len)
{
    int r;
    stdfs_handle_t *hf=(stdfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return fwrite(buf, 1, len, hf->fp);
}


static int stdfs_seek(handle_t h, int offset)
{
    stdfs_handle_t *hf=(stdfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return fseek(hf->fp, offset, SEEK_SET);
}


static int stdfs_sync(handle_t h)
{
    stdfs_handle_t *hf=(stdfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return fflush(hf->fp);
}


static int stdfs_truncate(handle_t h, int len)
{
    int r;
    stdfs_handle_t *hf=(stdfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    r = 0;//truncate(hf->obj, len);
    
    return r;
}


static int stdfs_size(handle_t h)
{
    int r;
    long curPos,size;
    struct stat st;
    stdfs_handle_t *hf=(stdfs_handle_t*)h;
    
    if(!hf || !hf->fp) {
        return -1;
    }
    
    r = fstat(hf->fp, &st);
    if(r) {
        return -1;
    }
    
    return st.st_size;
}


static int stdfs_length(char *path)
{
    int r;
    struct stat st;
    stdfs_handle_t *hf=(stdfs_handle_t*)h;
    
    if(!path || !length) {
        return -1;
    }
    
    r = stat(path, &st);
    if(r) {
        return -1;
    }
    return  st.st_size;
}

static int stdfs_exist(char *path)
{
    int r;
    struct stat st;
    stdfs_handle_t *hf=(stdfs_handle_t*)h;
    
    if(!path || !length) {
        return -1;
    }
    
    r = stat(path, &st);
    if(r==0) {
        return 1;
    }
    return 0;
}



#include "<dirent.h>"

static int stdfs_remove(char *path)
{
    return remove(path);
}


static int stdfs_mkdir(char *path)
{
    return 0;//_mkdir(path);
}


static int stdfs_rmdir(char *path)
{
    return 0;// _rmdir(path);    //linux
    //rmdir(path);              //win
}



static handle_t stdfs_opendir(char *path)
{
    return opendir(path);
}


static int stdfs_closedir(handle_t h)
{
    struct dirent *dir;

    dir = closedir((DIR*)path);
    if(dir) {
        info
    }
}


static int stdfs_readdir(handle_t h, fs_info_t *info)
{
    int slen;
    struct dirent *dir;
    
    dir = readdir((DIR*)h);
    if(!dir) {
        return -1;
    }
    
    info->size = dir->d_reclen;
    info->isdir = (dir->d_type==DT_DIR)?1:0;
    
    slen = strlen(dir->d_name);
    if(slen>=FS_NAME_MAX-1) {
        memcpy(info->fname, dir->d_name, FS_NAME_MAX-1);
        info->fname[FS_NAME_MAX-1] = 0;
    }
    else {
        strcpy(info->fname, de->d_name);
    }
    
    return 0;
}

static int stdfs_get_space(char *path, fs_space_t *sp)
{
    int r;
    struct statfs diskInfo;
    
    r = statfs(path, &diskInfo);
    if(r==0) {
        sp->free = diskInfo.f_bfree*diskInfo.f_bsize;
        sp->total = diskInfo.f_blocks*diskInfo.f_bsize;
    }
    
    return r;//GetLastError();
}


static int stdfs_last_err(void)
{
    return 0;//GetLastError();
}



fs_driver_t stdfs_driver={
    
    stdfs_init,
    stdfs_mount,
    stdfs_umount,
    stdfs_format,
    stdfs_open,
    stdfs_close,
    stdfs_read,
    stdfs_write,
    stdfs_seek,
    stdfs_sync,
    stdfs_truncate,
    stdfs_size,
    stdfs_length,
    stdfs_exist,
    stdfs_remove,
    stdfs_mkdir,
    stdfs_rmdir,
    stdfs_opendir,
    stdfs_readdir,
    stdfs_get_space,
    stdfs_last_err,
};
#endif


