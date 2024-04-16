#ifndef __B64_Hx__
#define __B64_Hx__

#include <stdint.h>

int b64_encode(uint8_t *dst, size_t dlen, size_t *olen, const uint8_t *src, size_t slen);
int b64_decode(uint8_t *dst, size_t dlen, size_t *olen, const uint8_t *src, size_t slen);

#endif
