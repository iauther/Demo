#ifndef __TYPES_Hx__
#define __TYPES_Hx__

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include "error.h"

#ifndef _WIN32
#include "platform.h"
#endif


typedef int8_t      INT8,  int8,  int8_t,  s8,  S8;
typedef int16_t     INT16, int16, int16_t, s16, S16;
typedef int32_t     INT32, int32, int32_t, s32, S32;
typedef int64_t     INT64, int64, int64_t, s64, S64;

typedef float       f32, F32;
typedef double      f64, F64;

#ifdef _WIN32
typedef uint8_t     UINT8,  INT8U,  uint8,  u8,  U8;
typedef uint16_t    UINT16, INT16U, uint16, u16, U16;
typedef uint32_t    UINT32, INT32U, uint32, u32, U32;
typedef uint64_t    UINT64, INT64U, uint64, u64, U64;
#else
typedef uint8_t     UINT8,  INT8U,  uint8,  BYTE,  u8,  U8;
typedef uint16_t    UINT16, INT16U, uint16, WORD,  u16, U16;
typedef uint32_t    UINT32, INT32U, uint32, DWORD, u32, U32;
typedef uint64_t    UINT64, INT64U, uint64, QWORD, u64, U64;

typedef uint8_t     *PBYTE, *PUINT8,   *PINT8U;
typedef uint16_t    *PWORD, *PUINT16,  *PINT16U;
typedef uint32_t    *PDWORD, *PUINT32, *PINT32U;

typedef uint8_t	    BOOL;

#endif

#define ALIGN_TO(addr,n) ((U32)(addr) + ((((U32)(addr))%(n))?((n)-(((U32)(addr))%(n))):0))

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (1)
#endif

#define SWAP16(x) ((x<<8)|(x>>8))


enum {
    MODE_POLL=0,
    MODE_IT,
    MODE_DMA,
};


typedef void*  handle_t;

#ifndef MAX
#define MAX(a,b)    ((a>b)?a:b)
#endif

#ifndef MIN
#define MIN(a,b)    ((a<b)?a:b)
#endif

#ifndef ABS
#define ABS(a)      (((a)>=0)?(a):((a)*(-1)))
#endif


#define offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

typedef struct {
    U16             year;
    U8              mon;
    U8              day;
    U8              week;
}date_s_t;

typedef struct {
    U8              hour;
    U8              min;
    U8              sec;
}time_s_t;

typedef struct {
    date_s_t        date;
    time_s_t        time;
}date_time_t;

typedef struct {
    U8              *buf;
    int             blen;
    int             dlen;
}buf_t;
    
typedef struct {
    void            *buf;
    U32             blen;
    U32             dlen;
}node_t;
typedef struct _lnode {
    node_t          data;
    struct _lnode   *prev;
    struct _lnode   *next;
}lnode_t;

typedef struct {
    U16             x;
    U16             y;
    U16             w;
    U16             h;
}rect_t;

#endif
