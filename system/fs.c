#include "fs.h"
#include "log.h"
#include "paras.h"
#include "protocol.h"
#include "cfg.h"
#include "log.h"

//https://toutiao.io/posts/w4mrk4/preview




typedef struct {
    fs_driver_t *drv;
    FS_DEV      dev;
    FS_TYPE     tp;
}fx_handle_t;

typedef struct {
    handle_t    fp;
    fx_handle_t *fs;
}file_handle_t;


typedef struct {
    fx_handle_t *fs;
    char        path[20];
}fs_handle_t;


#define FS_MAX    5

static fs_handle_t fsHandle[FS_MAX]={NULL};

////////////////////////////////////////////////////////////////////////////////

extern fs_driver_t fatfs_driver;
extern fs_driver_t yaffs2_driver;
extern fs_driver_t stdfs_driver;
static fs_driver_t *fs_drivers[FS_MAX]={
#ifdef USE_FATFS
    &fatfs_driver,
#else
    NULL,
#endif
    
#ifdef USE_YAFFS
    &yaffs2_driver,
#else
    NULL,
#endif

#ifdef USE_STDFS
    &stdfs_driver,
#else
    NULL,
#endif
};



static void mk_p_dir(fx_handle_t *h, char *path)
{
    char *p;
    char tmp[50];
    
    strcpy(tmp, path);
    p = strrchr(tmp, '/');
    if(p) {
        p[1] = 0;
        h->drv->mkdir(path);
    }
}


static handle_t fx_init(FS_DEV dev, FS_TYPE tp)
{
    int i=0,r=0,sz;
    fx_handle_t *h=malloc(sizeof(fx_handle_t));

    if(!h) {
        return NULL;
    }
    
    if(!fs_drivers[tp]) {
        LOGE("fs not support!\n");
        return NULL;
    }
    
    h->drv = fs_drivers[tp];
    h->dev = dev;
    h->tp  = tp;
    
    r = h->drv->init(h->dev, 0, 0xffff);
    if(r) {
        free(h);
        return NULL;
    }
    
    return h;
}


static int fx_deinit(handle_t h)
{
    int r=0,tp;
    fx_handle_t *fh=(fx_handle_t*)h;
    
    if(!fh) {
        return -1;
    }
    
    if(r) {
        return r;
    }
    free(fh);
    
    return 0;
}


static int fx_mount(handle_t fs, char *path)
{
    int i=0,r=0;
    fx_handle_t *fh=(fx_handle_t*)fs;
    
    if(!fh || !path) {
        return -1;
    }
    
    r = fh->drv->mount(fh->dev, path);
    if(r<0) {
        LOGD("fs mount failed 1, format now...\n");
        r = fh->drv->format(path);
        if(r<0) {
            LOGE("fs format failed\n");
            return -1;
        }
        
        r = fh->drv->mount(fh->dev, path);
        if(r<0) {
            LOGE("fs mount failed 2, exit!\n");
            return -1;
        }
    }
    
    return 0;
}


static int fx_umount(handle_t fs)
{
    int i=0,r=0;
    fx_handle_t *fh=(fx_handle_t*)fs;
    
    if(!fh) {
        return -1;
    }
    
    r = fh->drv->umount(fh->dev);
    
    return r;
}


static handle_t fx_open(handle_t fs, char *path, FS_MODE mode)
{
    file_handle_t *h;
    fx_handle_t *fh=(fx_handle_t*)fs;
    
    if(!fh || !path) {
        return NULL;
    }
    
    h = (file_handle_t*)malloc(sizeof(file_handle_t));
    if(!h) {
        LOGE("fs_open malloc failed\n");
        return NULL;
    }
    
    if(mode==FS_MODE_CREATE) {
        mk_p_dir(fh, path);
    }
    h->fp = fh->drv->open(path, mode);
    
    if(!h->fp) {
        LOGE("%s open failed.\n", path);
        free(h);
        return NULL;
    }
    h->fs = fs;
    
    return h;
}


static int fx_close(handle_t file)
{
    int r;
    file_handle_t *fh=(file_handle_t*)file;
    
    if(!fh) {
        return -1;
    }
    
    r = fh->fs->drv->close(fh->fp);
    if(r) {
        LOGE("fs_close failed\n");
        return -1;
    }
    free(fh);
    
    return r;
}


static int fx_size(handle_t file)
{
    file_handle_t *fh=(file_handle_t*)file;
    
    if(!fh) {
        return -1;
    }
    
    return fh->fs->drv->size(fh->fp); 
}


static int fx_read(handle_t file, void *buf, int buflen)
{
    int r=0,sz;
    file_handle_t *fh=(file_handle_t*)file;
    
    if(!fh || !buf || !buflen) {
        return -1;
    }
    
    if(!buf || !buflen) {
        sz = fh->fs->drv->size(fh->fp);
        return sz;
    }
    else {
        r = fh->fs->drv->read(fh->fp, buf, buflen);
    }

    return r;
}


int fx_write(handle_t file, void *buf, int buflen, int sync)
{
    int r=0,wl,sz;
    file_handle_t *fh=(file_handle_t*)file;
    
    if(!fh || !buf || !buflen) {
        return -1;
    }
    
    wl = fh->fs->drv->write(fh->fp, buf, buflen);
    if(wl>0 && sync>0) {
        r = fh->fs->drv->sync(fh->fp);
    }
    
    return wl;
}


static int fx_append(handle_t file, void *buf, int buflen, int sync)
{
    int r=0,sz;
    file_handle_t *fh=(file_handle_t*)file;
    
    if(!fh || !buf || !buflen) {
        return -1;
    }
    
    sz = fh->fs->drv->size(fh->fp);
    fh->fs->drv->seek(fh->fp, sz);
    r = fh->fs->drv->write(fh->fp, buf, buflen);
    if(r==0 && sync>0) {
        r = fh->fs->drv->sync(fh->fp);
    }
    
    return r;
}


static int fx_sync(handle_t file)
{
    int r=0;
    file_handle_t *fh=(file_handle_t*)file;
    
    if(!fh) {
        return -1;
    }
    
    r = fh->fs->drv->sync(fh->fp);
    
    return r;
}


static int fx_seek(handle_t file, int offset)
{
    int r;
    handle_t hd;
    file_handle_t *fl=(file_handle_t*)file;
    
    if(!fl) {
        return -1;
    }
    
    return  fl->fs->drv->seek(file, offset);
}


static int fx_length(handle_t fs, char *path)
{
    int r=0,length;
    fx_handle_t *fh=(fx_handle_t*)fs;

    if(!fh || !path) {
        return -1;
    }
    
    return  fh->drv->length(path);
}

static int fx_exist(handle_t fs, char *path)
{
    int r=0,length;
    fx_handle_t *fh=(fx_handle_t*)fs;

    if(!fh || !path) {
        return -1;
    }
    
    return  fh->drv->exist(path);
}



static int fx_remove(handle_t fs, char *path)
{
    int r=0;
    fx_handle_t *fh=(fx_handle_t*)fs;

    if(!fh || !path) {
        return -1;
    }
    
    return fh->drv->remove(path);
}


static int fx_get_space(handle_t fs, char *path, fs_space_t *sp)
{
    int r;
    fx_handle_t *fh=(fx_handle_t*)fs;
    
    if(!fh) {
        return -1;
    }
    
    r = fh->drv->get_space(path, sp);
    
    return r;
}


static int fx_scan(handle_t fs, char *path)
{
    int r;
    handle_t hd;
    fs_info_t info;
    fx_handle_t *fh=(fx_handle_t*)fs;
    
    if(!fh || !path) {
        return -1;
    }
    
    hd = fh->drv->opendir(path);
    if(!hd) {
        return -1;
    }
    
    while(1) {
        r = fh->drv->readdir(hd, &info);
        if(r==0) {
            LOGD("___dir:%d, flen:%d fname:%s\n", info.isdir, info.size, info.fname);
        }
        else {
            break;
        }
    }
    fh->drv->closedir(hd);
    
    return 0;
}


static int fx_save(handle_t fs, char *path, void *buf, int len)
{
    int r,wl;
    handle_t fl;
    
    if(!fs || !path || !buf || !len) {
        return -1;
    }
    
    fl = fx_open(fs, path, FS_MODE_CREATE);
    if(!fl) {
        LOGE("fs_save, open %s failed\n", path);
        return -1;
    }
    
    wl = fs_write(fl, buf, len, 0);
    LOGD("fs_save len: %d\n", wl);
    
    fs_close(fl);
    
    return 0;
}


static int fx_print(handle_t fs, char *path, int str_print)
{
    int i,rl,tl=0;
    handle_t fl;
    U8 tmp[100];
    
    if(!fs || !path) {
        return -1;
    }
    
    fl = fx_open(fs, path, FS_MODE_RW);
    if(!fl) {
        LOGE("fs open %s failed\n", path);
        return -1;
    }
    
    while(1) {
        rl = fs_read(fl, tmp, sizeof(tmp));
        if(rl>0) {

            if(str_print) {
                LOGD("%s\n", tmp);
            }
            else {
                for(i=0; i<rl; i++) {
                    LOGD("0x%02x,", tmp[i]);
                    if(i>0 && i%10==0) {
                        LOGD("\n");
                    }
                }
            }
            
            tl += rl;
            if(rl<sizeof(tmp)) {
                LOGD("fs_read total len: %d\n", tl);
                break;
            }
        }
    }
    
    fs_close(fl);
    
    return 0;
}
/////////////////////////////////////////////////////////////////////
static int do_mnount(int id, FS_DEV dev, FS_TYPE tp, char *mount_dir)
{
    int r;
    handle_t fs;
    
    fs = fx_init(dev, tp);
    if(fs) {
        r = fx_mount(fs, mount_dir);
        if(r==0) {
            fsHandle[id].fs = fs;
            strcpy(fsHandle[id].path, mount_dir);
        }
    }
    
    return r;
}
static handle_t find_fs(char *path)
{
    int i;
    
    for(i=0; i<FS_MAX; i++) {
        if(strstr(path, fsHandle[i].path)) {
            return fsHandle[i].fs;
        }
    }
    
    return NULL;
}
static handle_t find_mnt_fs(char *path)
{
    int i;
    
    for(i=0; i<FS_MAX; i++) {
        if(strcmp(path, fsHandle[i].path)==0) {
            return fsHandle[i].fs;
        }
    }
    
    return NULL;
}


int fs_init(void)
{
    int i,r;
    handle_t fs;
    
    for(i=0; i<FS_MAX; i++) {
        fsHandle[i].fs = NULL;
        fsHandle[i].path[0] = 0;
    }
    
    r  = do_mnount(0, DEV_SDMMC,  SDMMC_FS_TYPE,  SDMMC_MNT_PT);
    if(r) {
        return -1;
    }
    
    //r = do_mnount(1, DEV_SFLASH, SFLASH_FS_TYPE, SFLASH_MNT_PT);
    
    return r;
}

int fs_deinit(void)
{
    return 0;
}


handle_t fs_open(char *path, FS_MODE mode)
{
    handle_t fs=find_fs(path);
    
    if(!fs) {
        LOGE("___ media not mounted!\n");
        return NULL;
    }
    
    return fx_open(fs, path, mode);
}


handle_t fs_openx(U8 id, FS_MODE mode)
{
    handle_t fs;
    
    if(id>=FILE_MAX) {
        return NULL;
    }
    
    fs = find_fs((char*)filesPath[id]);
    if(!fs) {
        LOGE("___ media not mounted!\n");
        return NULL;
    }
    
    return fx_open(fs, (char*)filesPath[id], mode);
}


int fs_close(handle_t file)
{
    return fx_close(file);
}

int fs_size(handle_t file)
{
    return fx_size(file);
}

int fs_read(handle_t file, void *buf, int buflen)
{
    return fx_read(file, buf, buflen);
}

int fs_write(handle_t file, void *buf, int buflen, int sync)
{
    return fx_write(file, buf, buflen, sync);
}

int fs_append(handle_t file, void *buf, int buflen, int sync)
{
    return fx_append(file, buf, buflen, sync);
}


int fs_remove(char *path)
{
    handle_t fs=find_fs(path);
    
    if(!fs) {
        LOGE("___ media not mounted!\n");
        return NULL;
    }
    
    return fx_remove(fs, path);
}

int fs_sync(handle_t file)
{
    return fx_sync(file);
}
int fs_seek(handle_t file, int offset)
{
    return fx_seek(file, offset);
}

int fs_scan(char *path)
{
    handle_t fs=find_fs(path);
    
    if(!fs) {
        LOGE("___ media not mounted!\n");
        return NULL;
    }
    
    return fx_scan(fs, path);
}


int fs_readx(int id, void *buf, int len)
{
    int rlen;
    handle_t file;
    
    file = fs_openx(id, FS_MODE_RW);
    if(!file) {
        return -1;
    }
    
    rlen = fs_read(file, buf, len);
    fs_close(file);
    
    return rlen;
}


int fs_savex(int id, void *buf, int len)
{
    if(id>=FILE_MAX || !buf || !len) {
        return -1;
    }
    
    return fs_save((char*)filesPath[id], buf, len);
}



int fs_save(char *path, void *buf, int len)
{
    handle_t fs=find_fs(path);
    
    if(!fs) {
        LOGE("___ media not mounted!\n");
        return NULL;
    }
    
    return fx_save(fs, path, buf, len);
}

int fs_length(char *path)
{
    handle_t fs=find_fs(path);
    
    if(!fs) {
        LOGE("___ media not mounted!\n");
        return -1;
    }
    
    return fx_length(fs, path);
}


int fs_exist(char *path)
{
    handle_t fs=find_fs(path);
    
    if(!fs) {
        LOGE("___ media not mounted!\n");
        return -1;
    }
    
    return fx_exist(fs, path);
}


int fs_lengthx(int id)
{
    if(id>=FILE_MAX) {
        return -1;
    }
    
    return fs_length((char*)filesPath[id]);
}

int fs_get_space(char *path, fs_space_t *sp)
{
    handle_t fs=find_mnt_fs(path);
    
    if(!fs) {
        LOGE("___ fs_get_space %s is wrong!\n", path);
        return NULL;
    }
    
    return fx_get_space(fs, path, sp);
}





#define TEST_DEV_TYPE      DEV_SFLASH    //DEV_SDMMC
#define TEST_FS_TYPE       FS_FATFS
#define TEST_MOUNT_PT      "/sd"

int fs_test(void)
{
    return 0;
}



