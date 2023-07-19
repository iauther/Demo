#ifndef __TYPES_H__
#define __TYPES_H__


#ifdef _WIN32
	typedef   signed           char INT8, int8, int8_t, s8, S8;
	typedef   signed short     int  INT16, int16, int16_t, s16, S16;
	typedef   signed           int  INT32, int32, int32_t, s32, S32;

	typedef unsigned           char UINT8, uint8, u8, U8, uint8_t;// , BYTE;
	typedef unsigned short     int  UINT16, uint16, u16, U16, uint16_t;// , WORD;
	typedef unsigned           int  UINT32, uint32, u32, U32, uint32_t;// , DWORD;
#else
	typedef   signed           char INT8, int8, int8_t, s8, S8;
	typedef   signed short     int  INT16, int16, int16_t, s16, S16;
	typedef   signed           int  INT32, int32, int32_t, s32, S32;

	typedef unsigned           char UINT8, uint8, u8, U8, uint8_t , BYTE;
	typedef unsigned short     int  UINT16, uint16, u16, U16, uint16_t, WORD;
	typedef unsigned           int  UINT32, uint32, u32, U32, uint32_t, DWORD;

	typedef unsigned           char * PBYTE;
	typedef unsigned short     int  * PWORD;
	typedef unsigned           int  * PDWORD;

	typedef unsigned char	   BOOL, bool;
	typedef ((unsigned char)0) FALSE, false;
	typedef ((unsigned char)0) TRUE, true;
#endif

typedef float                  f32, F32;
typedef double                 f64, F64;

#endif 
