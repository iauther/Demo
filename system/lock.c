#include "lock.h"
#ifdef _WIN32
#include <windows.h>
#else
#include "incs.h"
#endif


/////////////////////////////////////////////////////
handle_t lock_init(void)
{
    handle_t h=NULL;

#ifdef _WIN32
    h = CreateMutex(NULL, FALSE, NULL);
#endif

#ifdef OS_KERNEL
    h = osMutexNew(NULL);
    if(!h) {
        return NULL;
    }
#endif

    return h;
}


int lock_on(handle_t h)
{
    int r=0;
    
    if(!h) {
        return -1;
    }
    
#ifdef _WIN32
    r = WaitForSingleObject(h, INFINITE);
#endif

#ifdef OS_KERNEL
    r = osMutexAcquire(h, osWaitForever);
#endif
    
    if(r) {
        return -1;
    }

    return 0;
}


int lock_off(handle_t h)
{
    int r=0;

#ifdef _WIN32
    r = ReleaseMutex((HANDLE)h);
#endif

#ifdef OS_KERNEL
    r = osMutexRelease(h);
#endif

    return r;
}


int lock_free(handle_t h)
{
    int r=0;

#ifdef _WIN32
    CloseHandle((HANDLE)h);
#endif

#ifdef OS_KERNEL
    r = osMutexDelete(h);
#endif

    return r;
}



