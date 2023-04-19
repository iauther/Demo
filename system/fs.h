#ifndef __FS_Hx__
#define __FS_Hx__

#ifdef __cplusplus
 extern "C" {
#endif


int fs_init(void);
int fs_read(int id, void *buf, int buflen);
int fs_write(int id, void *buf, int buflen);
int fs_remove(int id);
int fs_sync(int id);
int fs_test(void);

#ifdef __cplusplus
}
#endif
#endif /*__fatfs_H */
