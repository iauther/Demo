/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */




#ifndef _IOTX_COMMON_HMAC_H_
#define _IOTX_COMMON_HMAC_H_

#include <string.h>
#include <stdint.h>


#define KEY_IOPAD_SIZE        (64)
#define MD5_DIGEST_SIZE       (16)
#define SHA1_DIGEST_SIZE      (20)
#define SHA256_DIGEST_SIZE    (32)


#ifdef __cplusplus
extern "C" {
#endif

void utils_hmac_md5(char *msg, int msg_len, char *digest, char *key, int key_len);
void utils_hmac_sha1(char *msg, int msg_len, char *digest, char *key, int key_len);
void utils_hmac_sha1_hex(char *msg, int msg_len, char *digest, char *key, int key_len);
void utils_hmac_sha1_raw(char *msg, int msg_len, char *digest, char *key, int key_len);
void utils_hmac_sha1_base64(char *msg, int msg_len, char *key, int key_len, char *digest, int *digest_len);
void utils_hmac_sha256(char *msg, int msg_len, char *digest, char *key, int key_len);

#ifdef __cplusplus
}
#endif
    
    
#endif

