#include "fs.h"
#include "log.h"
#include "paras.h"
#include "protocol.h"
#include "cfg.h"
#include "log.h"
#include "list.h"


//https://toutiao.io/posts/w4mrk4/preview




typedef struct {
    fs_driver_t *drv;
    FS_DEV      dev;
    FS_TYPE     tp;
}fx_handle_t;

typedef struct {
    handle_t    fp;
    fx_handle_t *h;
}file_handle_t;


typedef struct {
    fx_handle_t *h;
    char        path[20];
}fs_handle_t;


fs_handle_t fsHandle[DEV_MAX][FS_MAX]={NULL};

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
    char *p,*x;
    handle_t hdir;
    char *tmp=malloc(strlen(path)+10);
    
    if(!tmp) {
        LOGE("___ mk_p_dir malloc failed\n");
        return;
    }
    
    x = tmp;
    strcpy(tmp, path);
    while(1) {
        p = strchr(x,  '/');
        if(p) {
            x = p+1;
            *p = 0;
            if(strlen(tmp)) {
                hdir = h->drv->opendir(tmp);
                if(!hdir) {
                    //LOGD("___ mkdir %s\n", tmp);
                    h->drv->mkdir(tmp);
                }
                else {
                    h->drv->closedir(hdir);
                }
            }
            *p = '/';
        }
        else {
            break;
        }
    }
    
    free(tmp);
}


static fx_handle_t* fx_init(FS_DEV dev, FS_TYPE tp)
{
    int i=0,r=0,sz;
    fx_handle_t *h=malloc(sizeof(fx_handle_t));

    if(!h) {
        LOGE("fx_init malloc failed\n");
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


static int fx_deinit(fx_handle_t *h)
{
    int r=0;
    
    if(r) {
        return r;
    }
    free(h);
    
    return 0;
}


static int fx_mount(fx_handle_t *h, char *path)
{
    int r=0;
    
    r = h->drv->mount(h->dev, path);
    if(r<0) {
        LOGD("mount %s failed, format now...\n", path);
        r = h->drv->format(h->dev, path);
        if(r) {
            LOGE("format %s failed, %d\n", path, r);
            return -1;
        }
        
        r = h->drv->mount(h->dev, path);
        if(r<0) {
            LOGE("mount %s failed %d, exit!\n", path, r);
            return -1;
        }
    }
    
    return 0;
}


static int fx_umount(fx_handle_t *h)
{
    int r=0;
    
    r = h->drv->umount(h->dev);
    
    return r;
}


static handle_t fx_open(fx_handle_t *h, char *path, FS_MODE mode)
{
    file_handle_t *fh;
    
    fh = (file_handle_t*)malloc(sizeof(file_handle_t));
    if(!fh) {
        LOGE("___ fx_open malloc failed\n");
        return NULL;
    }
    
    if(mode==FS_MODE_CREATE) {
        mk_p_dir(h, path);
    }
    fh->fp = h->drv->open(path, mode);
    
    if(!fh->fp) {
        LOGE("%s open failed.\n", path);
        free(fh);
        return NULL;
    }
    fh->h = h;
    
    return fh;
}


static int fx_close(file_handle_t *h)
{
    int r;
    
    r = h->h->drv->close(h->fp);
    if(r) {
        LOGE("fs_close failed\n");
        return -1;
    }
    free(h);
    
    return r;
}


static int fx_size(file_handle_t *h)
{    
    return h->h->drv->size(h->fp);
}


static int fx_read(file_handle_t *h, void *buf, int buflen)
{
    return h->h->drv->read(h->fp, buf, buflen);
}


int fx_write(file_handle_t *h, void *buf, int buflen, int sync)
{
    int r=0,wl;
    
    wl = h->h->drv->write(h->fp, buf, buflen);
    if(wl>0 && sync>0) {
        r = h->h->drv->sync(h->fp);
    }
    
    return wl;
}


static int fx_append(file_handle_t *h, void *buf, int buflen, int sync)
{
    int r=0,sz;
    
    sz = h->h->drv->size(h->fp);
    h->h->drv->seek(h->fp, sz);
    r = h->h->drv->write(h->fp, buf, buflen);
    if(r==0 && sync>0) {
        r = h->h->drv->sync(h->fp);
    }
    
    return r;
}


static int fx_sync(file_handle_t *h)
{
    return  h->h->drv->sync(h->fp);
}


static int fx_seek(file_handle_t *h, int offset)
{
    return  h->h->drv->seek(h->fp, offset);
}


static int fx_length(fx_handle_t *h, char *path)
{
    return  h->drv->length(path);
}

static int fx_exist(fx_handle_t *h, char *path)
{   
    return  h->drv->exist(path);
}



static int fx_remove(fx_handle_t *h, char *path)
{
    return h->drv->remove(path);
}


static int fx_get_space(fx_handle_t *h, char *path, fs_space_t *sp)
{
    return h->drv->get_space(path, sp);
}


static int fx_scan(fx_handle_t *h, char *path, handle_t l, int *nfiles)
{
    int r;
    handle_t hd;
    fs_info_t *info=NULL;
    
    info = malloc(sizeof(fs_info_t));
    if(!info) {
        LOGE("___ fx_scan malloc failed\n");
        return -1;
    }
        
    hd = h->drv->opendir(path);
    if(!hd) {
        free(info);
        return -1;
    }
    
    while(1) {
        r = h->drv->readdir(hd, info);
        if(r==0) {
            int len=strlen(path)+strlen(info->fname)+2;
            char *tmp=malloc(len);
            
            if(tmp) {
                
                sprintf(tmp, "%s/%s", path, info->fname);
                if(info->isdir) {
                    //LOGD("___dir, %s\n", tmp);
                    fx_scan(h, tmp, l, nfiles);
                }
                else {
                    //LOGD("___file, %s/%s, %d\n", path, info->fname, info->size);
                    list_append(l, 1, tmp, len);
                    if(nfiles) (*nfiles)++;
                }
            
                free(tmp);
            }
        }
        else {
            break;
        }
    }
    
    free(info);
    h->drv->closedir(hd);
    
    return 0;
}


static int fx_save(fx_handle_t *h, char *path, void *buf, int len)
{
    int r,wl;
    handle_t fl;
    
    fx_remove(h, path);
    
    fl = fx_open(h, path, FS_MODE_CREATE);
    if(!fl) {
        LOGE("fs_save, open %s failed\n", path);
        return -1;
    }
    
    wl = fs_write(fl, buf, len, 0);
    LOGD("fs_save %s len: %d\n", path, wl);
    
    fs_close(fl);
    
    return wl;
}


static int fx_print(fx_handle_t *h, char *path, int str_print)
{
    int i,rl,tl=0;
    handle_t fl;
    U8 tmp[100];
    
    
    fl = fx_open(h, path, FS_MODE_RW);
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
static int do_mount(FS_DEV dev, FS_TYPE tp, char *mount_dir)
{
    int r;
    fx_handle_t *h;
    
    h = fx_init(dev, tp);
    if(h) {
        r = fx_mount(h, mount_dir);
        if(r==0) {
            fsHandle[dev][tp].h = h;
            strcpy(fsHandle[dev][tp].path, mount_dir);
        }
    }
    
    return r;
}
static fx_handle_t* find_hnd(char *path)
{
    int i,j;
    
    if(!path) return NULL;
    
    for(i=0; i<DEV_MAX; i++) {
        for(j=0; j<FS_MAX; j++) {
            if(fsHandle[i][j].h && strstr(path, fsHandle[i][j].path)) {
                return fsHandle[i][j].h;
            }
        }
    }
    
    return NULL;
}
static fx_handle_t* find_mnt_hnd(char *path)
{
    int i,j;
    
    if(!path) return NULL;
    
    for(i=0; i<DEV_MAX; i++) {
        for(j=0; j<FS_MAX; j++) {
            if(fsHandle[i][j].h && strcmp(path, fsHandle[i][j].path)==0) {
                return fsHandle[i][j].h;
            }
        }
    }
    
    return NULL;
}


int fs_init(void)
{
    int i,j,r;
    
    for(i=0; i<DEV_MAX; i++) {
        for(j=0; j<FS_MAX; j++) {
            fsHandle[i][j].h = NULL;
            fsHandle[i][j].path[0] = 0;
        }
    }
    
    r = do_mount(DEV_SDMMC,  SDMMC_FS_TYPE,  SDMMC_MNT_PT);
    r = do_mount(DEV_SFLASH, SFLASH_FS_TYPE, SFLASH_MNT_PT);
    
    return r;
}

int fs_deinit(void)
{
    return 0;
}


handle_t fs_open(char *path, FS_MODE mode)
{
    fx_handle_t* h=NULL;
    
    h = find_hnd(path);
    if(!h) {
        return NULL;
    }
    
    return fx_open(h, path, mode);
}


handle_t fs_openx(U8 id, FS_MODE mode)
{
    if(id>=FILE_MAX) {
        return NULL;
    }
    
    return fs_open((char*)filesPath[id], mode);
}


int fs_close(handle_t file)
{
    if(!file) {
        return -1;
    }
    
    return fx_close(file);
}

int fs_size(handle_t file)
{
    file_handle_t *h=(file_handle_t*)file;
    
    if(!h) {
        return -1;
    }
    
    return fx_size(h);
}


int fs_load(char *path, void *buf, int buflen)
{
    int len=-1;
    handle_t h=fs_open(path, FS_MODE_RW);
    
    if(h) {
        len = fs_read(h, buf, buflen);
        fs_close(h);
    }
    
    return len;
}


int fs_read(handle_t file, void *buf, int buflen)
{
    file_handle_t *h=(file_handle_t*)file;
    
    if(!h || !buf || !buflen) {
        return -1;
    }
    
    return fx_read(h, buf, buflen);
}

int fs_write(handle_t file, void *buf, int buflen, int sync)
{
    file_handle_t *h=(file_handle_t*)file;
    
    if(!h || !buf || !buflen) {
        return -1;
    }
    
    return fx_write(h, buf, buflen, sync);
}

int fs_append(handle_t file, void *buf, int buflen, int sync)
{
    file_handle_t *h=(file_handle_t*)file;
    
    if(!h || !buf || !buflen) {
        return -1;
    }
    
    return fx_append(h, buf, buflen, sync);
}

int fs_sync(handle_t file)
{
    file_handle_t *h=(file_handle_t*)file;
    
    if(!h) {
        return -1;
    }
    
    return fx_sync(h);
}

int fs_seek(handle_t file, int offset)
{
    file_handle_t *h=(file_handle_t*)file;
    
    if(!h) {
        return -1;
    }
    
    return fx_seek(h, offset);
}


int fs_remove(char *path)
{
    handle_t h=find_hnd(path);
    
    if(!h) {
        LOGE("___ fs_remove, %s not mounted!\n", path);
        return NULL;
    }
    
    return fx_remove(h, path);
}

int fs_scan(char *path, handle_t l, int *nfiles)
{
    handle_t h=find_hnd(path);
    
    if(!h) {
        LOGE("___ fs_scan, %s not mounted!\n", path);
        return NULL;
    }
    
    return fx_scan(h, path, l, nfiles);
}


int fs_print(char *path, int str_print)
{
    handle_t h=find_hnd(path);
    
    if(!h) {
        LOGE("___ fs_print, %s not mounted!\n", path);
        return NULL;
    }
    
    return fx_print(h, path, str_print);
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
    handle_t h=find_hnd(path);
    
    if(!h || !buf || !len) {
        LOGE("___ fs_save, %s not mounted!\n", path);
        return NULL;
    }
    
    return fx_save(h, path, buf, len);
}


int fs_length(char *path)
{
    handle_t h=find_hnd(path);
    
    if(!h) {
        return NULL;
    }
    
    return fx_length(h, path);
}


int fs_exist(char *path)
{
    handle_t h=find_hnd(path);
    
    if(!h) {
        return NULL;
    }
    
    return fx_exist(h, path);
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
    handle_t h=find_mnt_hnd(path);
    
    if(!h) {
        LOGE("___ fs_get_space %s\n", path?" not mounted!":"path is NULL!");
        return -1;
    }
    
    return fx_get_space(h, path, sp);
}


#include "rtc.h"
int fs_test(void)
{
    int r;
    handle_t hfile=NULL;
    datetime_t dt;
    char tt[60];
    char tmp[100];
    int files=0;
    char *mntDIR=SFLASH_MNT_PT; //SDMMC_MNT_PT
    
    
    //fs_init();
    r = do_mount(DEV_SFLASH, SFLASH_FS_TYPE, SFLASH_MNT_PT);
    
    r = rtc2_get_time(&dt);

    sprintf(tt, "%04d/%02d/%02d/%02d", dt.date.year, dt.date.mon, dt.date.day, 0);
    sprintf(tmp, "%s/%s.csv", mntDIR, tt);

    hfile = fs_open(tmp, FS_MODE_CREATE);
    if(!hfile) {
        LOGE("___fs_open %s failed\n", tmp);
        return -1;
    }
    
    fs_scan(mntDIR, NULL, &files);
    
    sprintf(tt, "11223344556677889900aabbccdd");
    fs_write(hfile, tt, strlen(tt), 0);
    fs_close(hfile);
    
    fs_print(tmp, 1);
    
    return 0;
}



