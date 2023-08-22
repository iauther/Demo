#ifndef __MQTT_SIGN_Hx__
#define __MQTT_SIGN_Hx__

#include <stdint.h>


#define PRODUCT_KEY_MAXLEN           (20)
#define PRODUCT_SECRET_MAXLEN        (64)
#define DEVICE_NAME_MAXLEN           (32)
#define DEVICE_SECRET_MAXLEN         (64)

#define HOST_NAME_MAXLEN             (100)

#define CLIENTID_MAXLEN              (150)
#define USERNAME_MAXLEN              (64)
#define PASSWORD_MAXLEN              (65)


typedef struct {
    char productKey[PRODUCT_KEY_MAXLEN+1];
    char productSecret[PRODUCT_SECRET_MAXLEN+1];
    char deviceName[DEVICE_NAME_MAXLEN+1];
    char deviceSecret[DEVICE_SECRET_MAXLEN+1];
    
    char hostName[HOST_NAME_MAXLEN+1];
    uint16_t port;
} dev_meta_t;


typedef struct {
    char clientId[CLIENTID_MAXLEN];
    char username[USERNAME_MAXLEN];
    char password[PASSWORD_MAXLEN];
} mqtt_sign_t;


typedef enum {
    SIGN_HMAC_MD5,
    SIGN_HMAC_SHA1,
    SIGN_HMAC_SHA256,
}SIGN_TYPE;





#ifdef __cplusplus
extern "C" {
#endif

int mqtt_sign(dev_meta_t *meta, mqtt_sign_t *sign, SIGN_TYPE sType);

#ifdef __cplusplus
}
#endif

#endif
