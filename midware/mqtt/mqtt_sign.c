/*
 * Copyright (C) 2015-2019 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mqtt_sign.h"
#include "utils_hmac.h"



//#define DIRECT_CLIENTID

#define SIGN_SOURCE_MAXLEN           (200)
#define TIMESTAMP_VALUE             "1690854796645"


static void hex_to_str(uint32_t *input, int input_len, char *output)
{
    char *zEncode = "0123456789ABCDEF";
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





int mqtt_sign(dev_meta_t *meta, mqtt_sign_t *sign, SIGN_TYPE sType)
{
    char deviceId[PRODUCT_KEY_MAXLEN + DEVICE_NAME_MAXLEN + 2] = {0};
    char tmp[SIGN_SOURCE_MAXLEN] = {0};
    int res;

    if (!meta || !sign) {
        return -1;
    }
    
    memset(sign, 0, sizeof(mqtt_sign_t));
    if ((strlen(meta->productKey) > PRODUCT_KEY_MAXLEN) || (strlen(meta->deviceName) > DEVICE_NAME_MAXLEN) ||
        (strlen(meta->deviceSecret) > DEVICE_SECRET_MAXLEN)) {
        return -1;
    }

    
    if(sType==SIGN_HMAC_SHA256) {
        snprintf(deviceId, sizeof(deviceId), "%s&%s", meta->deviceName, meta->productKey);
        snprintf(sign->clientId, sizeof(sign->clientId), "%s|securemode=3,signmethod=hmacsha256,timestamp=%s|", deviceId, TIMESTAMP_VALUE);
        
        snprintf(sign->username, sizeof(sign->username), "%s&%s", meta->deviceName, meta->productKey);
        snprintf(tmp, sizeof(tmp), "clientId%sdeviceName%sproductKey%stimestamp%s", deviceId, meta->deviceName, meta->productKey, TIMESTAMP_VALUE);
        
        utils_hmac_sha256(tmp, strlen(tmp), sign->password, meta->deviceSecret, strlen(meta->deviceSecret));
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
        
        snprintf(sign->username, sizeof(sign->username), "%s&%s", meta->deviceName, meta->productKey);
        snprintf(tmp, sizeof(tmp), "clientId%sdeviceName%sproductKey%stimestamp%s", deviceId, meta->deviceName, meta->productKey, TIMESTAMP_VALUE);
        
        if(sType==SIGN_HMAC_SHA1) {
            utils_hmac_sha1(tmp, strlen(tmp), sign->password, meta->deviceSecret, strlen(meta->deviceSecret));
        }
        else {  //SIGN_HMAC_MD5
            utils_hmac_md5(tmp, strlen(tmp), sign->password, meta->deviceSecret, strlen(meta->deviceSecret));
        }
    }
    
    return 0;
}


