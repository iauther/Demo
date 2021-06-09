#ifndef MD5_H  
#define MD5_H  

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
	unsigned int count[2];
	unsigned int state[4];
	unsigned char buffer[64];
}md5_ctx_t;


#define F(x,y,z) ((x & y) | (~x & z))  
#define G(x,y,z) ((x & z) | (y & ~z))  
#define H(x,y,z) (x^y^z)  
#define I(x,y,z) (y ^ (x | ~z))  
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))  

#define FF(a,b,c,d,x,s,ac) \
	{ \
		a += F(b, c, d) + x + ac; \
		a = ROTATE_LEFT(a, s); \
		a += b; \
	}

#define GG(a,b,c,d,x,s,ac) \
	{ \
		a += G(b, c, d) + x + ac; \
		a = ROTATE_LEFT(a, s); \
		a += b; \
	}

#define HH(a,b,c,d,x,s,ac) \
	{ \
		a += H(b, c, d) + x + ac; \
		a = ROTATE_LEFT(a, s); \
		a += b; \
	}

#define II(a,b,c,d,x,s,ac) \
	{ \
		a += I(b, c, d) + x + ac; \
		a = ROTATE_LEFT(a, s); \
		a += b; \
	}

void md5_init(md5_ctx_t* context);
void md5_update(md5_ctx_t* context, unsigned char* input, unsigned int inputlen);
void md5_final(md5_ctx_t* context, unsigned char digest[16]);
void md5_transform(unsigned int state[4], unsigned char block[64]);
void md5_encode(unsigned char* output, unsigned int* input, unsigned int len);
void md5_decode(unsigned int* output, unsigned char* input, unsigned int len);

int md5_calc(void* p1, int len1, void* p2, int len2, char* md5);

#ifdef __cplusplus
}
#endif



#endif  
