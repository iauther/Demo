#include "fs.h"
#include "dal/nand.h"
#include "yaffs_nand_drv.h"
#include "protocol.h"
#include "cfg.h"
#include "log.h"

//#include "fatfs.h"
//#include "littlefs.h"

#ifdef _WIN32
#include<unistd.h>
static FILE* allFd[PARA_MAX]={NULL};
#else
#include "yaffsfs.h"
static int allFd[PARA_MAX]={0};
#endif



//https://toutiao.io/posts/w4mrk4/preview
static char *allFiles[PARA_MAX]={
    "/mdf.json"
    "/calCoef.bin",
    "/sysSett.bin",
};



int fs_init(void)
{
    int i=0,r=0;

#ifdef _WIN32
    FILE *fd;
    struct stat st;
    while(i++<PARA_MAX) {
        fd = fopen(allFiles[i], O_CREAT|O_RDWR, S_IREAD|S_IWRITE);
        if(fd) {
            fstat(fileno(fd), &st);
            if(st.st_size==0) {
                //create file
                //fwrite(fd, data, len);
                fflush(fd);
            }
            
            allFd[i] = fd;
        }
    }
#else
    #ifdef USE_YAFFS
        int fd;
        struct yaffs_stat stat;
        struct yaffs_dev *dev;
        dev = yaffs_nand_init(MOUNT_POINT, 0, NAND_FS_START, NAND_FS_LEN);
        
        if(yaffs_mount(MOUNT_POINT)<0) {
            r = yaffs_format(MOUNT_POINT, 1, 1, 1);
            if(r) {
                return r;
            }
            r = yaffs_mount(MOUNT_POINT);
        }
        
        while(i++<PARA_MAX) {
            fd = yaffs_open(allFiles[i], O_CREAT|O_RDWR, S_IREAD|S_IWRITE);
            if(fd>=0) {
                yaffs_fstat(fd, &stat);
                if(stat.st_size==0) {
                    //create file
                    //yaffs_write(fd, data, len);
                    //yaffs_flush(fd);
                }
                
                allFd[i] = r;
            }
        }
    #elif defined USE_FATFS
        
    #endif
#endif
    
    return 0;
}


int fs_deinit(void)
{
    int i=0,r=0;

#ifdef _WIN32
    while(i++<PARA_MAX) {
        r = fclose(allFd[i]);
        allFd[i] = NULL;
    }
#else
    #ifdef USE_YAFFS
        while(i++<PARA_MAX) {
            r = yaffs_close(allFd[i]);
            allFd[i] = -1;
        }
    #elif defined USE_FATFS
        
    #endif
#endif
    
    return r;
}


int fs_read(int id, void *buf, int buflen)
{
    int r=0;
    
#ifdef _WIN32
    FILE* fd=allFd[id];
    if(id<0 || id>=PARA_MAX || !fd) {
        return -1;
    }
    fseek(fd, 0, SEEK_SET);
    r = fread(fd, buf, buflen);
#else
    #ifdef USE_YAFFS
        int fd=allFd[id];
        if(id<0 || id>=PARA_MAX || fd>=0) {
            return -1;
        }
        
        yaffs_lseek(fd, 0, SEEK_SET);
        r = yaffs_read(fd, buf, buflen);
    #elif defined USE_FATFS
        
    #endif
#endif
    
    return r;
}


int fs_write(int id, void *buf, int buflen)
{
    int r=0;
    
#ifdef _WIN32
    FILE* fd=allFd[id];
    if(id<0 || id>=PARA_MAX || !fd) {
        return -1;
    }
    ftruncate(fd, 0);
    r = fwrite(fd, buf, buflen);
#else
    #ifdef USE_YAFFS
        int fd=allFd[id];
        if(id<0 || id>=PARA_MAX || fd>=0) {
            return -1;
        }
        
        yaffs_ftruncate(fd, 0);
        r = yaffs_write(fd, buf, buflen);
    #elif defined USE_FATFS
        
    #endif
#endif
    
    return r;
}


int fs_remove(int id)
{
    int r=0;
    
#ifdef _WIN32
    FILE* fd=allFd[id];
    if(id<0 || id>=PARA_MAX || !fd) {
        return -1;
    }
    fclose(fd);
    r = remove(allFiles[fd], buf, buflen);
#else
    #ifdef USE_YAFFS
        int fd=allFd[id];
        if(id<0 || id>=PARA_MAX || fd>=0) {
            return -1;
        }
        
        r = yaffs_funlink(fd);
    #elif defined USE_FATFS
        
    #endif
#endif
    
    return r;
}


int fs_sync(int id)
{
    int r=0;
    
#ifdef _WIN32
    FILE* fd=allFd[id];
    if(id<0 || id>=PARA_MAX || !fd) {
        return -1;
    }
    
    r = fflush(fd);
#else
    #ifdef USE_YAFFS
        int fd=allFd[id];
        if(id<0 || id>=PARA_MAX || fd>=0) {
            return -1;
        }
        
        r = yaffs_fsync(fd);
    #elif defined USE_FATFS
        
    #endif
#endif
    
    return r;
}


static void print_file(char *path)
{
    int fd,r;
    char tmp[50];
    
#ifndef _WIN32
    #ifdef USE_YAFFS
        struct yaffs_stat stat;
        
        fd = yaffs_open(path, O_RDONLY, S_IREAD);
        if(fd<0) {
            LOGD("____%s open failed\n", path);
            return;
        }
        
        yaffs_fstat(fd, &stat);
        r = yaffs_read(fd, tmp, 100);
        LOGD("____:%s", tmp);
        
        r = yaffs_close(fd);
    #endif
#endif
    
}


int fs_test(void)
{
    int i=0,r,fd,rl;
    char *path="/cfg/test.txt";
    char *ptr="hello 123\n";
    
#ifndef _WIN32
    #ifdef USE_YAFFS
        struct yaffs_dev *dev;
        struct yaffs_stat stat;
        
        dev = yaffs_nand_init(MOUNT_POINT, 0, NAND_FS_START, NAND_FS_LEN);
        
        r = -1;
        r = yaffs_mount(MOUNT_POINT);
        if(r<0) {
            r = yaffs_format(MOUNT_POINT, 1, 1, 1);
            if(r<0) {
                return r;
            }
            r = yaffs_mount(MOUNT_POINT);
        }
        
        
        //print_file(path);
        //return 0;
        
        fd = yaffs_open(path, O_CREAT|O_RDWR, S_IREAD|S_IWRITE);
        if(fd>=0) {
            yaffs_fstat(fd, &stat);
            if(stat.st_size==0) {
                r = yaffs_write(fd, ptr, strlen(ptr)+1);
                //r = yaffs_flush(fd);
            }
            r = yaffs_close(fd);
            
            print_file(path);
        }
    #endif
#endif
    
    return 0;
}



