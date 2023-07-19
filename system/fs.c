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
}fs_handle_t;

typedef struct {
    handle_t    fp;
    fs_handle_t *fs;
}file_handle_t;




static fs_handle_t *fileHandle[FILE_MAX]={NULL};
static const char *allFiles[FILE_MAX]={
    "/cfg/mdf.json",
    "/cfg/coef.dat",
    "/cfg/sett.cfg",
    
    "/log/r.log",
    "/bin/app.bin",
    "/bin/boot.bin",
};
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



static void mk_p_dir(fs_handle_t *h, char *path)
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


handle_t fs_init(FS_DEV dev, FS_TYPE tp)
{
    int i=0,r=0,sz;
    fs_handle_t *h=malloc(sizeof(fs_handle_t));

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


int fs_deinit(handle_t h)
{
    int r=0,tp;
    fs_handle_t *fh=(fs_handle_t*)h;
    
    if(!fh) {
        return -1;
    }
    
    if(r) {
        return r;
    }
    free(fh);
    
    return 0;
}


int fs_mount(handle_t fs, char *path)
{
    int i=0,r=0;
    fs_handle_t *fh=(fs_handle_t*)fs;
    
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


int fs_umount(handle_t fs)
{
    int i=0,r=0;
    fs_handle_t *fh=(fs_handle_t*)fs;
    
    if(!fh) {
        return -1;
    }
    
    r = fh->drv->umount(fh->dev);
    
    return r;
}


handle_t fs_open(handle_t fs, char *path, FS_MODE mode)
{
    file_handle_t *h;
    fs_handle_t *fh=(fs_handle_t*)fs;
    
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


int fs_close(handle_t file)
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


int fs_size(handle_t file)
{
    file_handle_t *fh=(file_handle_t*)file;
    
    if(!fh) {
        return -1;
    }
    
    return fh->fs->drv->size(fh->fp); 
}


int fs_read(handle_t file, void *buf, int buflen)
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


int fs_write(handle_t file, void *buf, int buflen, int sync)
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


int fs_append(handle_t file, void *buf, int buflen, int sync)
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


int fs_sync(handle_t file)
{
    int r=0;
    file_handle_t *fh=(file_handle_t*)file;
    
    if(!fh) {
        return -1;
    }
    
    r = fh->fs->drv->sync(fh->fp);
    
    return r;
}


int fs_seek(handle_t file, int offset)
{
    int r;
    handle_t hd;
    file_handle_t *fl=(file_handle_t*)file;
    
    if(!fl) {
        return -1;
    }
    
    return  fl->fs->drv->seek(file, offset);
}


int fs_remove(handle_t fs, char *path)
{
    int r=0;
    fs_handle_t *fh=(fs_handle_t*)fs;

    if(!fh || !path) {
        return -1;
    }
    
    return fh->drv->remove(path);
}


int fs_get_space(handle_t fs, fs_space_t *sp)
{
    int r;
    fs_handle_t *fh=(fs_handle_t*)fs;
    
    if(!fh) {
        return -1;
    }
    
    r = fh->drv->get_space("", sp);
    
    return r;
}


int fs_scan(handle_t fs, char *path)
{
    int r;
    handle_t hd;
    fs_info_t info;
    fs_handle_t *fh=(fs_handle_t*)fs;
    
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


int fs_save(handle_t fs, char *path, void *buf, int len)
{
    int r,wl;
    handle_t fl;
    
    if(!fs || !path || !buf || !len) {
        return -1;
    }
    
    fl = fs_open(fs, path, FS_MODE_CREATE);
    if(!fl) {
        LOGE("fs_save, open %s failed\n", path);
        return -1;
    }
    
    wl = fs_write(fl, buf, len, 0);
    LOGD("fs_save len: %d\n", wl);
    
    fs_close(fl);
    
    return 0;
}


int fs_print(handle_t fs, char *path, int str_print)
{
    int i,rl,tl=0;
    handle_t fl;
    U8 tmp[100];
    
    if(!fs || !path) {
        return -1;
    }
    
    fl = fs_open(fs, path, FS_MODE_RW);
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


#define TEST_DEV_TYPE      DEV_SFLASH    //DEV_SDMMC
#define TEST_FS_TYPE       FS_FATFS
#define TEST_MOUNT_PT      "/sd"

int fs_test(void)
{
    int r,rlen;
    U8 tmp[100];
    char *xpath="/sd/app.upg";
    char *test="hello every one! good morning ...";
    handle_t fl,fs;
    
    LOGE("\n\nfs_test\n");
    
    fs = fs_init(TEST_DEV_TYPE, TEST_FS_TYPE);
    if(!fs) {
        LOGE("____ myFs.sd init failed\n");
        return -1;
    }
    

    r = fs_mount(fs, TEST_MOUNT_PT);
    if(r) {
        LOGE("____ fs mount failed\n");
        return -1;
    }

    fs_scan(fs, TEST_MOUNT_PT);
    
    
    r = fs_save(fs, xpath, test, strlen(test)+1);
    r = fs_print(fs, xpath, 0);
    
    
#if 0
    fl = fs_open(fs, path, FS_MODE_RW);
    if(!fl) {
        LOGE("____ fs open %s failed\n", path);
        return -1;
    }
        
    rlen = fs_read(fl, tmp, 100);
    if(rlen>0) {
        LOGD("_____%d\n", rlen);
    }
    
    fs_close(fl);
#endif
    
    fs_deinit(fs);
    
    return 0;
}



