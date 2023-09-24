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




#define PRODUCT_KEY_MAXLEN           (20)
#define PRODUCT_SECRET_MAXLEN        (64)
#define DEVICE_NAME_MAXLEN           (32)
#define DEVICE_SECRET_MAXLEN         (64)

#define HOST_URL_MAXLEN             (100)

#define CLIENTID_MAXLEN              (150)
#define USERNAME_MAXLEN              (64)
#define PASSWORD_MAXLEN              (65)


typedef struct {
    char productKey[PRODUCT_KEY_MAXLEN+1];
    char productSecret[PRODUCT_SECRET_MAXLEN+1];
    char deviceName[DEVICE_NAME_MAXLEN+1];
    char deviceSecret[DEVICE_SECRET_MAXLEN+1];

    char hostUrl[HOST_URL_MAXLEN];
    uint16_t port;
} meta_data_t;


typedef struct {
    char clientId[CLIENTID_MAXLEN];
    char username[USERNAME_MAXLEN];
    char password[PASSWORD_MAXLEN];
} sign_data_t;


typedef enum {
    SIGN_HMAC_MD5,
    SIGN_HMAC_SHA1,
    SIGN_HMAC_SHA256,
}SIGN_TYPE;


#ifdef __cplusplus
extern "C" {
#endif

int utils_hmac_sign(meta_data_t *meta, sign_data_t *sign, SIGN_TYPE sType);


#ifdef __cplusplus
}
#endif
    
    
#endif

