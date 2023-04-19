#include "yaffs_osglue.h"
#include "yaffs_trace.h"
#include "cmsis_os2.h"
#include "log.h"


static int yaffsfs_lastError;
unsigned int yaffs_trace_mask = YAFFS_TRACE_BAD_BLOCKS | YAFFS_TRACE_ALWAYS | YAFFS_TRACE_ERROR;

u32 yaffsfs_CurrentTime(void)
{
    return osKernelGetTickCount();
}


void yaffsfs_SetError(int err)
{
    yaffsfs_lastError = err;
}


int yaffsfs_GetLastError(void)
{
	return yaffsfs_lastError;
}


void *yaffsfs_malloc(size_t size)
{
    return malloc(size);
}


void yaffsfs_free(void *ptr)
{
    return free(ptr);
}


int yaffsfs_CheckMemRegion(const void *addr, size_t size, int write_request)
{
    (void) size;
	(void) write_request;

	if(!addr)
		return -1;
	return 0;
    
}


void yaffsfs_OSInitialisation(void)
{
    
}
void yaffsfs_Lock(void)
{
    
}


void yaffsfs_Unlock(void)
{
    
}


#if defined(__CC_ARM)
size_t strnlen(const char *s, size_t maxlen)
{
    size_t i;
    for (i=0;i<maxlen; i++) {
        if(s[i] == 0) {
            break;
        }
    }
    return i;
}
#endif

void yaffs_bug_fn(const char *file_name, int line_no)
{
	LOGE("yaffs bug detected %s:%d", file_name, line_no);
	while(1);
}


