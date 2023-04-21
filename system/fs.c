#include "fs.h"
#include "cfg.h"
#include "log.h"
#include "paras.h"
#include "protocol.h"

//https://toutiao.io/posts/w4mrk4/preview


static fhand_t allFd[FILE_MAX]={FNULL};

static const char *allFiles[FILE_MAX]={
    "/cfg/mdf.json",
    "/cfg/coef.dat",
    "/cfg/sett.cfg",
    
    "/log/r.log",
    "/bin/app.bin",
    "/bin/boot.bin",
};

static void mk_p_dir(const char *path)
{
    char *p;
    char tmp[50];
    
    strcpy(tmp, path);
    p = strrchr(tmp, '/');
    if(p) {
        p[1] = 0;
        _fmkdir(tmp, S_IREAD|S_IWRITE);
    }
}




int fs_init(void)
{
    stat_t st;
    fhand_t fd;
    int i=0,r=0;

#ifndef _WIN32
    _finit(MOUNT_POINT, 0, NAND_FS_START, NAND_FS_LEN);
    
    r = _fmount(MOUNT_POINT);
    if(r<0) {
        LOGD("fs mount failed 1, format now...\n");
        r = _format(MOUNT_POINT, 1, 1, 1);
        if(r<0) {
            LOGE("fs format failed\n");
            return -1;
        }
        
        r = _fmount(MOUNT_POINT);
        if(r<0) {
            LOGE("fs mount failed 2, exit!\n");
            return -1;
        }
    }
#endif
        
    while(i<FILE_MAX) {
        mk_p_dir(allFiles[i]);
        fd = _fopen(allFiles[i], O_CREAT|O_RDWR, S_IREAD|S_IWRITE);
        if(fd<0) {
            LOGE("%d, %s create err: %d\n", i, allFiles[i], _last_err());
            i++;
            continue;
        }
        
        _fstat(fd, &st);
        LOGD("%d, %s size: %ld\n", i, allFiles[i], st.st_size);
        if(st.st_size==0) {
            if(i==FILE_CH) {
                _fwrite(fd, chJson, strlen(chJson)+1);
                _fflush(fd);
            }
        }
        
        allFd[i] = fd;
        i++;
    }
    
    return 0;
}


int fs_deinit(void)
{
    int i=0,r=0;

    while(i++<FILE_MAX) {
        r = _fclose(allFd[i]);
        allFd[i] = FNULL;
    }
    
    return r;
}


int fs_read(int id, void *buf, int buflen)
{
    int r=0;
    stat_t st;
    fhand_t fd=allFd[id];
    
    if(id<0 || id>=FILE_MAX || FINVALID(fd)) {
        return -1;
    }
    
    if(!buf || !buflen) {
        r = _fstat(fd, &st);
        if(r) {
            return r;
        }
        return st.st_size;
    }
    else {
        _fseek(fd, 0, SEEK_SET);
        r = _fread(fd, buf, buflen);
    }

    return r;
}


int fs_write(int id, void *buf, int buflen)
{
    int r=0;
    fhand_t fd=allFd[id];
    
    if(id<0 || id>=FILE_MAX || FINVALID(fd)) {
        return -1;
    }
    _ftruncate(fd, 0);
    r = _fwrite(fd, buf, buflen);
    r = _fflush(fd);
    
    return r;
}


int fs_remove(int id)
{
    int r=0;
    fhand_t fd=allFd[id];
    
    if(id<0 || id>=FILE_MAX || FINVALID(fd)) {
        return -1;
    }
    _fclose(fd);
    r = _fremvoe(allFiles[id]);
    
    return r;
}


int fs_sync(int id)
{
    int r=0;
    fhand_t fd=allFd[id];
    
    if(id<0 || id>=FILE_MAX || FINVALID(fd)) {
        return -1;
    }
    
    r = _fflush(fd);
    
    return r;
}


static void print_file(char *path)
{
    int r;
    fhand_t fd;
    char tmp[50];
    
#ifndef _WIN32
    #ifdef USE_YAFFS
        struct yaffs_stat stat;
        
        fd = _fopen(path, O_RDONLY, S_IREAD);
        if(fd<0) {
            LOGD("____%s open failed\n", path);
            return;
        }
        
        _fstat(fd, &stat);
        r = _fread(fd, tmp, 100);
        LOGD("____:%s", tmp);
        
        r = _fclose(fd);
    #endif
#endif
    
}


int fs_test(void)
{
    int i=0,r,fd,rl;
    char *path="/data/test.txt";
    char *ptr="hello 123 oo!\n";
    
#ifndef _WIN32
    #ifdef USE_YAFFS
        stat_t stat;
        
        _finit(MOUNT_POINT, 0, NAND_FS_START, NAND_FS_LEN);
        
        r = -1;
        r = _fmount(MOUNT_POINT);
        if(r<0) {
            r = _format(MOUNT_POINT, 1, 1, 1);
            if(r<0) {
                return r;
            }
            r = _fmount(MOUNT_POINT);
        }
        
        mk_p_dir(path);
        
        //print_file(path);
        //return 0;
        
        fd = _fopen(path, O_CREAT|O_RDWR, S_IREAD|S_IWRITE);
        if(fd>=0) {
            _fstat(fd, &stat);
            if(stat.st_size==0) {
                r = _fwrite(fd, ptr, strlen(ptr)+1);
                //r = _fflush(fd);
            }
            r = _fclose(fd);
            
            print_file(path);
        }
    #endif
#endif
    
    return 0;
}



