#include "ff.h"
#include "fs.h"
#include "log.h"


#ifdef USE_FATFS

typedef struct {
    FS_DEV      dev;
    FIL         fp;
}fatfs_handle_t;

typedef struct {
    FATFS       fs;
    char        disk[8];
    char        mpath[64];      //mount path
}fatfs_info_t;
static fatfs_info_t *fatInfo[DEV_MAX]={NULL};


#if FF_MULTI_PARTITION 
PARTITION VolToPart[FF_VOLUMES] = {
	{DEV_SDMMC,  0},
    {DEV_UDISK,  0},
    {DEV_SFLASH,  0},
    {DEV_PFLASH,  0},
};
#endif

static fatfs_info_t* get_info(char *path)
{
    int i;
    fatfs_info_t *info=NULL;
    
    for(i=0; i<DEV_MAX; i++) {
        if(fatInfo[i] && strstr(path, fatInfo[i]->mpath)) {
            info = fatInfo[i];
            break;
        }
    }
    
    return info;
}

static int fatfs_init(FS_DEV dev, U32 start, U32 len)
{
    return 0;
}


static int fatfs_mount(FS_DEV dev, char *path)
{
    int r;
    char tmp[64];
    fatfs_info_t *info=NULL;
    char *disk[DEV_MAX]={"0:","1:","2:","3:"};
    
    if(fatInfo[dev]) {
        LOGD("device is already mounted!\n");
        return 0;
    }
    
    info = (fatfs_info_t*)malloc(sizeof(fatfs_info_t));;
    if(!info) {
        return -1;
    }
    
    r = f_mount(&info->fs, disk[dev], 1);
    if(r!=FR_OK) {
        free(info);
        return -1;
    }
    
    strcpy(info->disk, disk[dev]);
    strcpy(info->mpath, path);
    
    fatInfo[dev] = info;
        
    return 0;
}


static int fatfs_umount(FS_DEV dev)
{    
    if(!fatInfo[dev]) {
        return -1;
    }
    
    f_unmount(fatInfo[dev]->disk);
    free(fatInfo[dev]);
    fatInfo[dev] = NULL;
    
    return 0;
}


static int fatfs_format(char *path)
{
    MKFS_PARM para;
            
    para.fmt = FM_EXFAT;
    para.n_fat = 0;
    para.n_root = 0;
    para.au_size = 1024;
    return f_mkfs(path, &para, NULL, 1024);
}



static handle_t fatfs_open(char *path, FS_MODE mode)
{
    int r=-1;
    char tmp[100];
    BYTE flag;
    char *str=NULL;
    fatfs_info_t *info=NULL;
    fatfs_handle_t *h=(fatfs_handle_t*)malloc(sizeof(fatfs_handle_t));
    
    if(!h || !path) {
        return NULL;
    }
    
    info = get_info(path);
    if(!info) {
        LOGE("___ media not mount!\n");
        free(h); return NULL;
    }
    
    flag = FA_READ|FA_WRITE;
    if(mode==FS_MODE_CREATE) {
        flag |= FA_CREATE_ALWAYS;
    }
    else {
        flag |= FA_OPEN_ALWAYS;
    }
    
    snprintf(tmp, sizeof(tmp), "%s%s", info->disk, path+strlen(info->mpath));
    r = f_open(&h->fp, tmp, flag);
    if(r!=FR_OK) {
        LOGE("f_open %s failed, %d\n", tmp, r);
        free(h);
        return NULL;
    }
            
    return h;
}


static int fatfs_close(handle_t h)
{
    int r;
    fatfs_handle_t *hf=(fatfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    r = f_close(&hf->fp);
    free(hf);
    
    return r;
}


static int fatfs_read(handle_t h, void *buf, int len)
{
    int r;
    UINT rlen;
    fatfs_handle_t *hf=(fatfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    r = f_read(&hf->fp, buf, len, &rlen);
    if(r==FR_OK) {
        r = rlen;
    }
    
    return r;
}


static int fatfs_write(handle_t h, void *buf, int len)
{
    int r; UINT wlen=0;
    fatfs_handle_t *hf=(fatfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    r = f_write(&hf->fp, buf, len, &wlen);
    if(r==FR_OK) {
        r = wlen;
    }
    
    return r;
}


static int fatfs_seek(handle_t h, int offset)
{
    fatfs_handle_t *hf=(fatfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return f_lseek(&hf->fp, offset);
}


static int fatfs_sync(handle_t h)
{
    fatfs_handle_t *hf=(fatfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return f_sync(&hf->fp);
}


static int fatfs_truncate(handle_t h, int len)
{
    int r;
    fatfs_handle_t *hf=(fatfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    r = f_lseek(&hf->fp, len);
    if(r==FR_OK) {
        r = f_truncate(&hf->fp);
    }
    
    return r;
}


static int fatfs_size(handle_t h)
{
    int r;
    FILINFO info;
    fatfs_handle_t *hf=(fatfs_handle_t*)h;
    
    if(!hf) {
        return -1;
    }
    
    return f_size(&hf->fp);
}


static int fatfs_length(char *path)
{
    FRESULT fr;
    FILINFO fno;
 
    if(!path) {
        return -1;
    }
    
    fr = f_stat(path, &fno);
    if(fr==FR_OK) {
        return fno.fsize;
    }
    
    return -1;
}


static int fatfs_exist(char *path)
{
    FRESULT fr;
    FILINFO fno;
 
    if(!path) {
        return -1;
    }
    
    fr = f_stat(path, &fno);
    if(fr==FR_OK) {
        return 1;
    }
    
    return 0;
}


static int fatfs_remove(char *path)
{
    char tmp[100];
    fatfs_info_t *info=NULL;
    
    if(!path) {
        return -1;
    }
    
    info = get_info(path);
    if(!info) {
        LOGE("___ media not mount!\n");
        return -1;
    }
    
    snprintf(tmp, sizeof(tmp), "%s%s", info->disk, path+strlen(info->mpath));
    return f_unlink(tmp);
}


static int fatfs_mkdir(char *path)
{
    char tmp[100];
    fatfs_info_t *info=NULL;
    
    if(!path) {
        return -1;
    }
    
    info = get_info(path);
    if(!info) {
        LOGE("___ media not mount!\n");
        return -1;
    }
    
    snprintf(tmp, sizeof(tmp), "%s%s", info->disk, path+strlen(info->mpath));
    return f_mkdir(tmp);
}


static int fatfs_rmdir(char *path)
{
    char tmp[100];
    fatfs_info_t *info=NULL;
    
    if(!path) {
        return -1;
    }
    
    info = get_info(path);
    if(!info) {
        LOGE("___ media not mount!\n");
        return -1;
    }
    
    snprintf(tmp, sizeof(tmp), "%s%s", info->disk, path+strlen(info->mpath));
    return f_rmdir(tmp);
}


static handle_t fatfs_opendir(char *path)
{
    int i,r;
    char tmp[100];
    fatfs_info_t *info=NULL;
    handle_t h=malloc(sizeof(DIR));
    
    if(!h) {
        LOGE("fatfs_opendir malloc failed\n");
        return NULL;
    }
    
    info = get_info(path);
    if(!info) {
        LOGE("___ media not mount!\n");
        free(h); return NULL;
    }
    
    snprintf(tmp, sizeof(tmp), "%s%s", info->disk, path+strlen(info->mpath));
    r = f_opendir((DIR*)h, tmp);
    if(r) {
        LOGE("___f_opendir failed, %d\n", r);
        free(h); return NULL;
    }
    
    return h;
}


static int fatfs_closedir(handle_t h)
{
    int r;
    
    if(!h) {
        return -1;
    }
    
    r = f_closedir(((DIR*)h));
    if(r==0) {
        free(h);
    }
    
    return r;
}


static int fatfs_readdir(handle_t h, fs_info_t *info)
{
    int r;
    int slen;
    FILINFO inf;
    
    if(!h) {
        return -1;
    }
    
    r = f_readdir((DIR*)h, &inf);
    if (r != FR_OK) {
        LOGE("fatfs_readdir failed\n");
        return -1;
    }
    
    if (inf.fname[0] == 0) {
        LOGE("fatfs_readdir finished\n");
        return -1;
    }
    
    
    if (inf.fattrib & AM_DIR) {             /* Directory */
        info->isdir = 1;
    }
    else {
        info->isdir = 0;                    /* File */
    }
    
    info->size = inf.fsize;
    slen = strlen(inf.fname);
    if(slen>=FS_NAME_MAX-1) {
        memcpy(info->fname, inf.fname, FS_NAME_MAX-1);
        info->fname[FS_NAME_MAX-1] = 0;
    }
    else {
        strcpy(info->fname, inf.fname);
    }
    
    return 0;
}


static int fatfs_get_space(char *path, fs_space_t *sp)
{
    int r;
    FATFS *fs;
    DWORD f_clust,t_clust;
    fatfs_info_t *info=NULL;
    
    if(!path || !sp) {
        return -1;
    }
    
    info = get_info(path);
    if(!info) {
        LOGE("__ media not mounted!\n");
        return -1;
    }
    
    r = f_getfree(info->disk, &f_clust, &fs);
    if(r!=FR_OK) {
        LOGE("__ f_getfree failed\n\n");
        return -1;
    }
    
    sp->total = (fs->n_fatent-2) * fs->csize;
    sp->free  = f_clust*fs->csize;
    
    return 0;
}


static int fatfs_last_err(void)
{
    LOGD("last error not support");
    return 0;
}



static int fatfs_test(void)
{
    FIL  fp;
    FATFS fs;
    FRESULT r; 
    char *wBuf="hello 123!";
    char rBuf[40];
    char *path="0:123.txt";
    LBA_t plist[] = {50, 50, 0};
    UINT fn;
    #define WORKLEN  4096
    char *pbuf=(char*)malloc(WORKLEN);
    MKFS_PARM fmtPara={FM_EXFAT, 0, 0, 0, 0};
    
    if(pbuf) {
        r = f_mount(&fs,"0:",1);
        if (r != FR_OK){
            LOGD("mount fail , error code : %d\n", r);
            
            if(r==FR_NO_FILESYSTEM) {
                r = f_mkfs("0:", &fmtPara, pbuf, WORKLEN);
                //r = f_mkfs("0:", NULL, workBuf, sizeof(workBuf));
                if(r != FR_OK) {
                    LOGE("f_mkfs failed!\n");
                    return -1;
                }
                //r = f_mount(NULL,"0:",1);
                
                r = f_mount(&fs,"0:",1);
                if(r != FR_OK) {
                    LOGE("f_mount fail , error code : %d\n", r);
                    return -1;
                }
            }
        }
        LOGD("f_mount success!!!\n");

        f_unlink(path);
        
        r = f_open(&fp, path, FA_CREATE_NEW|FA_READ|FA_WRITE); 
        if (r==FR_OK) {
            r = f_write(&fp, wBuf, strlen(wBuf), &fn);
            if(r==FR_OK) {
                f_sync(&fp);
            }
        }
        
        f_lseek(&fp, 0);
        r = f_read(&fp, rBuf, sizeof(rBuf), &fn); 
        if(r==FR_OK) {
            LOGD("read: %s\n", rBuf);
        }

        f_close(&fp);
        
        f_unmount(path);
        
        free(pbuf);
    }
    
    return 0;
}


fs_driver_t fatfs_driver={
    fatfs_init,
    fatfs_mount,
    fatfs_umount,
    fatfs_format,
    fatfs_open,
    fatfs_close,
    fatfs_read,
    fatfs_write,
    fatfs_seek,
    fatfs_sync,
    fatfs_truncate,
    fatfs_size,
    fatfs_length,
    fatfs_exist,
    fatfs_remove,
    fatfs_mkdir,
    fatfs_rmdir,
    fatfs_opendir,
    fatfs_closedir,
    fatfs_readdir,
    fatfs_get_space,
    fatfs_last_err,
};
#endif


