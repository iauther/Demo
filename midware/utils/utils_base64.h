/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */




#ifndef _IOTX_COMMON_BASE64_H_
#define _IOTX_COMMON_BASE64_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int utils_base64encode(const uint8_t *data, uint32_t inputLength, uint32_t outputLenMax,
                              uint8_t *encodedData, uint32_t *outputLength);
int utils_base64decode(const uint8_t *data, uint32_t inputLength, uint32_t outputLenMax,
                              uint8_t *decodedData, uint32_t *outputLength);

#ifdef __cplusplus
}
#endif

#endif
