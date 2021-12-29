#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include "error.h"



typedef int8_t          S8;
typedef uint8_t         U8;
typedef int16_t         S16;
typedef uint16_t        U16;
typedef int32_t         S32;
typedef uint32_t        U32;
typedef int64_t         S64;
typedef uint64_t        U64;
typedef float           F32;
typedef double          F64;

typedef void*           handle_t;

#define MAX(a,b)    ((a>b)?a:b)
#define MIN(a,b)    ((a<b)?a:b)
#define ABS(a)      (((a)>=0)?(a):((a)*(-1)))
    
typedef struct {
    void            *ptr;
    U32             len;
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



#ifndef _WIN32
#include "platform.h"
#endif

#endif
