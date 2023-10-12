/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */




#include <string.h>

#include "log.h"
#include "utils_md5.h"
#include "utils_sha1.h"
#include "utils_sha256.h"
#include "utils_base64.h"
#include "utils_hmac.h"


void utils_hmac_md5(char *msg, int msg_len, char *digest, char *key, int key_len)
{
    if ((NULL == msg) || (NULL == digest) || (NULL == key)) {
        //LOGE("parameter is Null,failed!");
        return;
    }

    if (key_len > KEY_IOPAD_SIZE) {
        //LOGE("key_len > size(%d) of array", KEY_IOPAD_SIZE);
        return;
    }

    iot_md5_context context;
    uint8_t k_ipad[KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
    uint8_t k_opad[KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
    uint8_t out[MD5_DIGEST_SIZE];
    int i;

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i = 0; i < KEY_IOPAD_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /* perform inner MD5 */
    utils_md5_init(&context);                                      /* init context for 1st pass */
    utils_md5_starts(&context);                                    /* setup context for 1st pass */
    utils_md5_update(&context, k_ipad, KEY_IOPAD_SIZE);            /* start with inner pad */
    utils_md5_update(&context, (uint8_t *) msg, msg_len);    /* then text of datagram */
    utils_md5_finish(&context, out);                               /* finish up 1st pass */

    /* perform outer MD5 */
    utils_md5_init(&context);                              /* init context for 2nd pass */
    utils_md5_starts(&context);                            /* setup context for 2nd pass */
    utils_md5_update(&context, k_opad, KEY_IOPAD_SIZE);    /* start with outer pad */
    utils_md5_update(&context, out, MD5_DIGEST_SIZE);      /* then results of 1st hash */
    utils_md5_finish(&context, out);                       /* finish up 2nd pass */

    for (i = 0; i < MD5_DIGEST_SIZE; ++i) {
        digest[i * 2] = utils_bin2hex(out[i] >> 4);
        digest[i * 2 + 1] = utils_bin2hex(out[i]);
    }
}

void utils_hmac_sha1_hex(char *msg, int msg_len, char *digest, char *key, int key_len)
{
    if ((NULL == msg) || (NULL == digest) || (NULL == key)) {
        LOGE("parameter is Null,failed!");
        return;
    }

    if (key_len > KEY_IOPAD_SIZE) {
        LOGE("key_len > size(%d) of array", KEY_IOPAD_SIZE);
        return;
    }

    iot_sha1_context context;
    uint8_t k_ipad[KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
    uint8_t k_opad[KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
    uint8_t out[SHA1_DIGEST_SIZE];
    int i;

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i = 0; i < KEY_IOPAD_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /* perform inner SHA */
    utils_sha1_init(&context);                                      /* init context for 1st pass */
    utils_sha1_starts(&context);                                    /* setup context for 1st pass */
    utils_sha1_update(&context, k_ipad, KEY_IOPAD_SIZE);            /* start with inner pad */
    utils_sha1_update(&context, (uint8_t *) msg, msg_len);    /* then text of datagram */
    utils_sha1_finish(&context, out);                               /* finish up 1st pass */

    /* perform outer SHA */
    utils_sha1_init(&context);                              /* init context for 2nd pass */
    utils_sha1_starts(&context);                            /* setup context for 2nd pass */
    utils_sha1_update(&context, k_opad, KEY_IOPAD_SIZE);    /* start with outer pad */
    utils_sha1_update(&context, out, SHA1_DIGEST_SIZE);     /* then results of 1st hash */
    utils_sha1_finish(&context, out);                       /* finish up 2nd pass */
    memcpy(digest, out, SHA1_DIGEST_SIZE);
}

void utils_hmac_sha256(char *msg, int msg_len, char *digest, char *key, int key_len)
{
    if ((NULL == msg) || (NULL == digest) || (NULL == key)) {
        LOGE("parameter is Null,failed!");
        return;
    }

    if (key_len > KEY_IOPAD_SIZE) {
        LOGE("key_len > size(%d) of array", KEY_IOPAD_SIZE);
        return;
    }

    iot_sha256_context context;
    uint8_t k_ipad[KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
    uint8_t k_opad[KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
    uint8_t out[SHA256_DIGEST_SIZE];
    int i;

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i = 0; i < KEY_IOPAD_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /* perform inner SHA */
    utils_sha256_init(&context);                                      /* init context for 1st pass */
    utils_sha256_starts(&context);                                    /* setup context for 1st pass */
    utils_sha256_update(&context, k_ipad, KEY_IOPAD_SIZE);            /* start with inner pad */
    utils_sha256_update(&context, (uint8_t *) msg, msg_len);    /* then text of datagram */
    utils_sha256_finish(&context, out);                               /* finish up 1st pass */

    /* perform outer SHA */
    utils_sha256_init(&context);                              /* init context for 2nd pass */
    utils_sha256_starts(&context);                            /* setup context for 2nd pass */
    utils_sha256_update(&context, k_opad, KEY_IOPAD_SIZE);    /* start with outer pad */
    utils_sha256_update(&context, out, SHA256_DIGEST_SIZE);     /* then results of 1st hash */
    utils_sha256_finish(&context, out);                       /* finish up 2nd pass */

    for (i = 0; i < SHA256_DIGEST_SIZE; ++i) {
        digest[i * 2] = utils_bin2hex(out[i] >> 4);
        digest[i * 2 + 1] = utils_bin2hex(out[i]);
    }
}

void utils_hmac_sha1(char *msg, int msg_len, char *digest, char *key, int key_len)
{
    if ((NULL == msg) || (NULL == digest) || (NULL == key)) {
        LOGE("parameter is Null,failed!");
        return;
    }

    if (key_len > KEY_IOPAD_SIZE) {
        LOGE("key_len > size(%d) of array", KEY_IOPAD_SIZE);
        return;
    }

    iot_sha1_context context;
    uint8_t k_ipad[KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
    uint8_t k_opad[KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
    uint8_t out[SHA1_DIGEST_SIZE];
    int i;

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i = 0; i < KEY_IOPAD_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /* perform inner SHA */
    utils_sha1_init(&context);                                      /* init context for 1st pass */
    utils_sha1_starts(&context);                                    /* setup context for 1st pass */
    utils_sha1_update(&context, k_ipad, KEY_IOPAD_SIZE);            /* start with inner pad */
    utils_sha1_update(&context, (uint8_t *) msg, msg_len);    /* then text of datagram */
    utils_sha1_finish(&context, out);                               /* finish up 1st pass */

    /* perform outer SHA */
    utils_sha1_init(&context);                              /* init context for 2nd pass */
    utils_sha1_starts(&context);                            /* setup context for 2nd pass */
    utils_sha1_update(&context, k_opad, KEY_IOPAD_SIZE);    /* start with outer pad */
    utils_sha1_update(&context, out, SHA1_DIGEST_SIZE);     /* then results of 1st hash */
    utils_sha1_finish(&context, out);                       /* finish up 2nd pass */

    for (i = 0; i < SHA1_DIGEST_SIZE; ++i) {
        digest[i * 2] = utils_bin2hex(out[i] >> 4);
        digest[i * 2 + 1] = utils_bin2hex(out[i]);
    }
}

void utils_hmac_sha1_raw(char *msg, int msg_len, char *digest, char *key, int key_len)
{
    if ((NULL == msg) || (NULL == digest) || (NULL == key)) {
        LOGE("parameter is Null,failed!");
        return;
    }

    if (key_len > KEY_IOPAD_SIZE) {
        LOGE("key_len > size(%d) of array", KEY_IOPAD_SIZE);
        return;
    }

    iot_sha1_context context;
    uint8_t k_ipad[KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
    uint8_t k_opad[KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
    uint8_t out[SHA1_DIGEST_SIZE];
    int i;

    /* start out by storing key in pads */
    memset(k_ipad, 0, sizeof(k_ipad));
    memset(k_opad, 0, sizeof(k_opad));
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    /* XOR key with ipad and opad values */
    for (i = 0; i < KEY_IOPAD_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }

    /* perform inner SHA */
    utils_sha1_init(&context);                                      /* init context for 1st pass */
    utils_sha1_starts(&context);                                    /* setup context for 1st pass */
    utils_sha1_update(&context, k_ipad, KEY_IOPAD_SIZE);            /* start with inner pad */
    utils_sha1_update(&context, (uint8_t *) msg, msg_len);    /* then text of datagram */
    utils_sha1_finish(&context, out);                               /* finish up 1st pass */

    /* perform outer SHA */
    utils_sha1_init(&context);                              /* init context for 2nd pass */
    utils_sha1_starts(&context);                            /* setup context for 2nd pass */
    utils_sha1_update(&context, k_opad, KEY_IOPAD_SIZE);    /* start with outer pad */
    utils_sha1_update(&context, out, SHA1_DIGEST_SIZE);     /* then results of 1st hash */
    utils_sha1_finish(&context, out);                       /* finish up 2nd pass */

    memcpy(digest, out, SHA1_DIGEST_SIZE);
}

void utils_hmac_sha1_base64(char *msg, int msg_len, char *key, int key_len, char *digest, int *digest_len)
{
    char buf[SHA1_DIGEST_SIZE];
    utils_hmac_sha1_raw(msg, msg_len, buf, key, key_len);

    uint32_t outlen;
    utils_base64encode((uint8_t *)buf, SHA1_DIGEST_SIZE, *digest_len, (uint8_t *)digest, &outlen);
    *digest_len = outlen;
}
//////////////////////////////////////////////////////////////////////////////////////////////////



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils_hmac.h"



//#define DIRECT_CLIENTID

#define SIGN_SOURCE_MAXLEN           (200)
#define TIMESTAMP_VALUE             "1690854796645"


static void hex_to_str(uint32_t *input, int input_len, char *output)
{
    const char *zEncode = "0123456789ABCDEF";
    int i = 0, j = 0;

    for (i = 0; i < input_len; i++) {
        output[j++] = zEncode[(input[i]>>28) & 0xf];
        output[j++] = zEncode[(input[i]>>24) & 0xf];
        output[j++] = zEncode[(input[i]>>20) & 0xf];
        output[j++] = zEncode[(input[i]>>16) & 0xf];
        output[j++] = zEncode[(input[i]>>12) & 0xf];
        output[j++] = zEncode[(input[i]>>8) & 0xf];
        output[j++] = zEncode[(input[i]) & 0xf];
    }
    output[j] = 0;
}

#ifdef _WIN32
#include <windows.h>
int get_chipID(char *id, int len)
{
    INT32 dwBuf[4];

    __cpuidex(dwBuf, 1, 1);
    sprintf_s(id, len, "%08X%08X%08X", dwBuf[2], dwBuf[1], dwBuf[0]);
    //sprintf_s(id, len, "%s", "AABBCCDD");

    return 0;
}
#else
#include "dal.h"
int get_chipID(char *id, int len)
{
    mcu_info_t info;

    dal_get_info(&info);
    //hex_to_str(info.uniqueID, 3, id);
    snprintf(id, len, "%08X%08X%08X", info.uniqueID[2], info.uniqueID[1], info.uniqueID[0]);

    return 0;
}
#endif



int utils_hmac_sign(net_para_t *para, sign_data_t *sign, SIGN_TYPE sType)
{
    char deviceId[PRODUCT_KEY_MAXLEN + DEVICE_NAME_MAXLEN + 2] = {0};
    char tmp[SIGN_SOURCE_MAXLEN] = {0};
    int res;
    plat_para_t *plat=NULL;

    if (!para || !sign) {
        return -1;
    }
    plat = &para->para.plat;
    
    memset(sign, 0, sizeof(sign_data_t));
    if ((strlen(plat->prdKey) > PRODUCT_KEY_MAXLEN) || (strlen(plat->devKey) > DEVICE_NAME_MAXLEN) ||
        (strlen(plat->devSecret) > DEVICE_SECRET_MAXLEN)) {
        return -1;
    }
    
    if(sType==SIGN_HMAC_SHA256) {
        snprintf(deviceId, sizeof(deviceId), "%s&%s", plat->devKey, plat->prdKey);
        snprintf(sign->clientId, sizeof(sign->clientId), "%s|securemode=3,signmethod=hmacsha256,timestamp=%s|", deviceId, TIMESTAMP_VALUE);
        
        snprintf(sign->username, sizeof(sign->username), "%s&%s", plat->devKey, plat->prdKey);
        snprintf(tmp, sizeof(tmp), "clientId%sdeviceName%sproductKey%stimestamp%s", deviceId, plat->devKey, plat->prdKey, TIMESTAMP_VALUE);
        
        utils_hmac_sha256(tmp, strlen(tmp), sign->password, plat->devSecret, strlen(plat->devSecret));
    }
    else {
        get_chipID(deviceId, sizeof(deviceId));
        
#ifdef DIRECT_CLIENTID
        snprintf(sign->clientId, sizeof(sign->clientId), "%s", deviceId);
#else
        if(sType==SIGN_HMAC_SHA1) {
            snprintf(sign->clientId, sizeof(sign->clientId), "%s|securemode=2,signmethod=hmacsha1,timestamp=%s|", deviceId, TIMESTAMP_VALUE);
        }
        else {
            snprintf(sign->clientId, sizeof(sign->clientId), "%s|securemode=2,signmethod=hmacmd5,timestamp=%s|", deviceId, TIMESTAMP_VALUE);
        }
#endif
        
        snprintf(sign->username, sizeof(sign->username), "%s&%s", plat->devKey, plat->prdKey);
        snprintf(tmp, sizeof(tmp), "clientId%sdeviceName%sproductKey%stimestamp%s", deviceId, plat->devKey, plat->prdKey, TIMESTAMP_VALUE);
        
        if(sType==SIGN_HMAC_SHA1) {
            utils_hmac_sha1(tmp, strlen(tmp), sign->password, plat->devSecret, strlen(plat->devSecret));
        }
        else {  //SIGN_HMAC_MD5
            utils_hmac_md5(tmp, strlen(tmp), sign->password, plat->devSecret, strlen(plat->devSecret));
        }
    }
    
    return 0;
}

