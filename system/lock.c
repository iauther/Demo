#include "lock.h"
#ifdef _WIN32
#include <windows.h>
#else
#include "incs.h"
#endif

#ifdef OS_KERNEL
osMutexId_t mutex[LOCK_MAX];

typedef struct {
    osMutexId_t mutex_id;
}lock_handle_t;
#endif




int lock_staic_init(void)
{
    int i;

#ifdef OS_KERNEL
    for(i=0; i<LOCK_MAX; i++) {
        mutex[i] = osMutexNew(NULL);
    }
#endif
   
    return 0;
}


int lock_static_free(void)
{
    int i;
    
#ifdef OS_KERNEL
    for(i=0; i<LOCK_MAX; i++) {
        osMutexDelete(mutex[i]);
    }
#endif
    
    return 0;
}


int lock_static_hold(int id)
{
    int r=0;

#ifdef OS_KERNEL
    r = osMutexAcquire(mutex[id], 0);
#endif

    return r;
}


int lock_static_release(int id)
{
    int r=0;

#ifdef OS_KERNEL
    r = osMutexRelease(mutex[id]);
#endif

    return r;
}


/////////////////////////////////////////////////////
handle_t lock_dynamic_new(void)
{
    handle_t h=NULL;

#ifdef _WIN32
    h = CreateMutex(NULL, FALSE, NULL);
#endif

#ifdef OS_KERNEL
    lock_handle_t *lh=NULL;

    lh = calloc(1, sizeof(lock_handle_t));
    if(!lh) {
        return NULL;
    }
    lh->mutex_id = osMutexNew(NULL);
    h = lh;
#endif

    return h;
}


int lock_dynamic_hold(handle_t h)
{
    int r=0;
#ifdef _WIN32
    r = WaitForSingleObject(h, INFINITE);
#endif

#ifdef OS_KERNEL
    lock_handle_t *lh=(lock_handle_t*)h;
    if(!h) {
        return -1;
    }
    r = osMutexAcquire(lh->mutex_id, osWaitForever);
#endif

    return r;
}


int lock_dynamic_release(handle_t h)
{
    int r=0;

#ifdef _WIN32
    r = ReleaseMutex((HANDLE)h);
#endif

#ifdef OS_KERNEL
    lock_handle_t *lh=(lock_handle_t*)h;
    if(!h) {
        return -1;
    }
    r = osMutexRelease(lh->mutex_id);
#endif

    return r;
}


int lock_dynamic_free(handle_t h)
{
    int r=0;

#ifdef _WIN32
    CloseHandle((HANDLE)h);
#endif

#ifdef OS_KERNEL
    lock_handle_t *lh=(lock_handle_t*)h;

    if(!lh) {
        return -1;
    }
    
    r = osMutexDelete(lh->mutex_id);
    free(lh);
#endif

    return r;
}



