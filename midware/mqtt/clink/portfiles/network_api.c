/**
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 * @details
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmsis_os2.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_at_api.h"
#include <time.h>


//#define CORE_SYSDEP_MBEDTLS_ENABLED     /* enable ebedtls default */

#ifdef CORE_SYSDEP_MBEDTLS_ENABLED
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/platform.h"
#include "mbedtls/timing.h"

typedef struct {
    mbedtls_net_context          net_ctx;
    mbedtls_ssl_context          ssl_ctx;
    mbedtls_ssl_config           ssl_config;
    mbedtls_timing_delay_context timer_delay_ctx;
    mbedtls_x509_crt             x509_server_cert;
    mbedtls_x509_crt             x509_client_cert;
    mbedtls_pk_context           x509_client_pk;
} core_sysdep_mbedtls_t;
#endif /* #ifdef CORE_SYSDEP_MBEDTLS_ENABLED */

/**
 * @brief 网络上下文结构体定义
 */
typedef struct {
    uint32_t connect_timeout_ms;
    core_sysdep_socket_type_t socket_type;
    aiot_sysdep_network_cred_t *cred;
    uint8_t fd;
    char *host;
    uint16_t port;
#ifdef CORE_SYSDEP_MBEDTLS_ENABLED
    core_sysdep_psk_t psk;
    core_sysdep_mbedtls_t mbedtls;
#endif
} core_network_handle_t;

/* local function declaration */
static int32_t _core_sysdep_network_tcp_establish(core_network_handle_t *network_handle);



#ifdef CORE_SYSDEP_MBEDTLS_ENABLED

/* mbedtls内存管理函数, 带内存统计能力, 用户应根据设备实际平台设备内存管理回调函数 */
#define MBEDTLS_MEM_INFO_MAGIC  (0x12345678)
static unsigned int g_mbedtls_total_mem_used = 0;
static unsigned int g_mbedtls_max_mem_used = 0;

typedef struct {
    int magic;
    int size;
} mbedtls_mem_info_t;

/* 带内存统计的mbedtls calloc内存回调, 用户可去除内存统计 */
static void *_core_mbedtls_calloc(size_t n, size_t size)
{
    unsigned char *buf = NULL;
    mbedtls_mem_info_t *mem_info = NULL;

    if (n == 0 || size == 0) {
        return NULL;
    }

    buf = (unsigned char *)malloc(n * size + sizeof(mbedtls_mem_info_t));
    if (NULL == buf) {
        return NULL;
    } else {
        memset(buf, 0, n * size + sizeof(mbedtls_mem_info_t));
    }

    mem_info = (mbedtls_mem_info_t *)buf;
    mem_info->magic = MBEDTLS_MEM_INFO_MAGIC;
    mem_info->size = n * size;
    buf += sizeof(mbedtls_mem_info_t);

    g_mbedtls_total_mem_used += mem_info->size;
    if (g_mbedtls_total_mem_used > g_mbedtls_max_mem_used) {
        g_mbedtls_max_mem_used = g_mbedtls_total_mem_used;
    }
    /* printf("INFO -- mbedtls malloc: %p %d  total used: %d  max used: %d\r\n",
                       buf, (int)size, g_mbedtls_total_mem_used, g_mbedtls_max_mem_used); */

    return buf;
}

/* 带内存统计的mbedtls calloc内存回调, 用户可去除内存统计 */
static void _core_mbedtls_free(void *ptr)
{
    mbedtls_mem_info_t *mem_info = NULL;
    if (NULL == ptr) {
        return;
    }

    mem_info = (mbedtls_mem_info_t *)((unsigned char *)ptr - sizeof(mbedtls_mem_info_t));
    if (mem_info->magic != MBEDTLS_MEM_INFO_MAGIC) {
        printf("Warning - invalid mem info magic: 0x%x\r\n", mem_info->magic);
        return;
    }

    g_mbedtls_total_mem_used -= mem_info->size;
    /* printf("INFO -- mbedtls free: %p %d  total used: %d  max used: %d\r\n",
                       ptr, mem_info->size, g_mbedtls_total_mem_used, g_mbedtls_max_mem_used); */

    free(mem_info);
}

void core_sysdep_rand(uint8_t *output, uint32_t output_len);
/* mbedtls随机数发生器回调函数, 此处直接复用`core_sysdep_rand`, 用户应根据设备平台进行重新适配 */
static int _mbedtls_random(void *handle, unsigned char *output, size_t output_len)
{
    core_sysdep_rand(output, output_len);
    return 0;
}

/* 辅助函数, 检查host是否为ip地址 */
static uint8_t _host_is_ip(char *host)
{
    uint32_t idx = 0;

    if (strlen(host) >= 16) {
        return 0;
    }

    for (idx = 0; idx < strlen(host); idx++) {
        if ((host[idx] != '.') && (host[idx] < '0' || host[idx] > '9')) {
            return 0;
        }
    }

    return 1;
}

/* mbedtls log回调函数 */
static void _mbedtls_debug(void *ctx, int level, const char *file, int line, const char *str)
{
    ((void) level);
    if (NULL != ctx) {
        printf("%s\n", str);
    }
}

/* mbedtls网络数据发送回调函数, 直接调用AT网络发送接口 */
int mbedtls_net_send_at( void *ctx, const unsigned char *buf, size_t len )
{
    return aiot_at_nwk_send(0, buf, len, 0);
}

/* mbedtls网络数据接收回调函数, 直接调用AT网络接收接口 */
int mbedtls_net_recv_at( void *ctx, unsigned char *buf, size_t len )
{
    return aiot_at_nwk_recv(0, buf, len, 1000);
}

/* mbedtls网络数据接收回调函数, 直接调用AT网络接受接口 */
int mbedtls_net_recv_timeout_at( void *ctx, unsigned char *buf, size_t len, uint32_t timeout )
{
    return aiot_at_nwk_recv(0, buf, len, timeout);
}

static int32_t _core_sysdep_network_mbedtls_establish(core_network_handle_t *network_handle)
{
    int32_t res = 0;

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(0);
#endif /* #if defined(MBEDTLS_DEBUG_C) */
    mbedtls_net_init(&network_handle->mbedtls.net_ctx);
    mbedtls_ssl_init(&network_handle->mbedtls.ssl_ctx);
    mbedtls_ssl_config_init(&network_handle->mbedtls.ssl_config);
    mbedtls_platform_set_calloc_free(_core_mbedtls_calloc, _core_mbedtls_free);
    g_mbedtls_total_mem_used = g_mbedtls_max_mem_used = 0;

    if (network_handle->cred->max_tls_fragment == 0) {
        printf("invalid max_tls_fragment parameter\n");
        return STATE_PORT_TLS_INVALID_MAX_FRAGMENT;
    }

    printf("establish mbedtls connection with server(host='%s', port=[%u])\n", network_handle->host, network_handle->port);

    if (network_handle->cred->max_tls_fragment <= 512) {
        res = mbedtls_ssl_conf_max_frag_len(&network_handle->mbedtls.ssl_config, MBEDTLS_SSL_MAX_FRAG_LEN_512);
    } else if (network_handle->cred->max_tls_fragment <= 1024) {
        res = mbedtls_ssl_conf_max_frag_len(&network_handle->mbedtls.ssl_config, MBEDTLS_SSL_MAX_FRAG_LEN_1024);
    } else if (network_handle->cred->max_tls_fragment <= 2048) {
        res = mbedtls_ssl_conf_max_frag_len(&network_handle->mbedtls.ssl_config, MBEDTLS_SSL_MAX_FRAG_LEN_2048);
    } else if (network_handle->cred->max_tls_fragment <= 4096) {
        res = mbedtls_ssl_conf_max_frag_len(&network_handle->mbedtls.ssl_config, MBEDTLS_SSL_MAX_FRAG_LEN_4096);
    } else {
        res = mbedtls_ssl_conf_max_frag_len(&network_handle->mbedtls.ssl_config, MBEDTLS_SSL_MAX_FRAG_LEN_NONE);
    }

    if (res < 0) {
        printf("mbedtls_ssl_conf_max_frag_len error, res: -0x%04X\n", -res);
        return res;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        res = _core_sysdep_network_tcp_establish(network_handle);
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        /* TODO: add _core_sysdep_network_udp_establish */
    }

    if (res < STATE_SUCCESS) {
        return res;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        res = mbedtls_ssl_config_defaults(&network_handle->mbedtls.ssl_config, MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        res = mbedtls_ssl_config_defaults(&network_handle->mbedtls.ssl_config, MBEDTLS_SSL_IS_CLIENT,
                                          MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT);
    }

    if (res < 0) {
        printf("mbedtls_ssl_config_defaults error, res: -0x%04X\n", -res);
        return res;
    }

    mbedtls_ssl_conf_max_version(&network_handle->mbedtls.ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3,
                                 MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_min_version(&network_handle->mbedtls.ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3,
                                 MBEDTLS_SSL_MINOR_VERSION_3);
    mbedtls_ssl_conf_handshake_timeout(&network_handle->mbedtls.ssl_config,(MBEDTLS_SSL_DTLS_TIMEOUT_DFL_MIN * 2),
                                       (MBEDTLS_SSL_DTLS_TIMEOUT_DFL_MIN * 2 * 4));
    mbedtls_ssl_conf_rng(&network_handle->mbedtls.ssl_config, _mbedtls_random, NULL);
    mbedtls_ssl_conf_dbg(&network_handle->mbedtls.ssl_config, _mbedtls_debug, stdout);

    if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA) {
        if (network_handle->cred->x509_server_cert == NULL && network_handle->cred->x509_server_cert_len == 0) {
            printf("invalid x509 server cert\n");
            return STATE_PORT_TLS_INVALID_SERVER_CERT;
        }
        mbedtls_x509_crt_init(&network_handle->mbedtls.x509_server_cert);

        res = mbedtls_x509_crt_parse(&network_handle->mbedtls.x509_server_cert,
                                     (const unsigned char *)network_handle->cred->x509_server_cert, (size_t)network_handle->cred->x509_server_cert_len + 1);
        if (res < 0) {
            printf("mbedtls_x509_crt_parse server cert error, res: -0x%04X\n", -res);
            return STATE_PORT_TLS_INVALID_SERVER_CERT;
        }

        if (network_handle->cred->x509_client_cert != NULL && network_handle->cred->x509_client_cert_len > 0 &&
                network_handle->cred->x509_client_privkey != NULL && network_handle->cred->x509_client_privkey_len > 0) {
            mbedtls_x509_crt_init(&network_handle->mbedtls.x509_client_cert);
            mbedtls_pk_init(&network_handle->mbedtls.x509_client_pk);
            res = mbedtls_x509_crt_parse(&network_handle->mbedtls.x509_client_cert,
                                         (const unsigned char *)network_handle->cred->x509_client_cert, (size_t)network_handle->cred->x509_client_cert_len + 1);
            if (res < 0) {
                printf("mbedtls_x509_crt_parse client cert error, res: -0x%04X\n", -res);
                return STATE_PORT_TLS_INVALID_CLIENT_CERT;
            }
            res = mbedtls_pk_parse_key(&network_handle->mbedtls.x509_client_pk,
                                       (const unsigned char *)network_handle->cred->x509_client_privkey,
                                       (size_t)network_handle->cred->x509_client_privkey_len + 1, NULL, 0);
            if (res < 0) {
                printf("mbedtls_pk_parse_key client pk error, res: -0x%04X\n", -res);
                return STATE_PORT_TLS_INVALID_CLIENT_KEY;
            }
            res = mbedtls_ssl_conf_own_cert(&network_handle->mbedtls.ssl_config, &network_handle->mbedtls.x509_client_cert,
                                            &network_handle->mbedtls.x509_client_pk);
            if (res < 0) {
                printf("mbedtls_ssl_conf_own_cert error, res: -0x%04X\n", -res);
                return STATE_PORT_TLS_INVALID_CLIENT_CERT;
            }
        }
        mbedtls_ssl_conf_ca_chain(&network_handle->mbedtls.ssl_config, &network_handle->mbedtls.x509_server_cert, NULL);
    } else if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_SVRCERT_PSK) {
        static const int ciphersuites[1] = {MBEDTLS_TLS_PSK_WITH_AES_128_CBC_SHA};
        res = mbedtls_ssl_conf_psk(&network_handle->mbedtls.ssl_config,
                                   (const unsigned char *)network_handle->psk.psk, (size_t)strlen(network_handle->psk.psk),
                                   (const unsigned char *)network_handle->psk.psk_id, (size_t)strlen(network_handle->psk.psk_id));
        if (res < 0) {
            printf("mbedtls_ssl_conf_psk error, res = -0x%04X\n", -res);
            return STATE_PORT_TLS_CONFIG_PSK_FAILED;
        }

        mbedtls_ssl_conf_ciphersuites(&network_handle->mbedtls.ssl_config, ciphersuites);
    } else {
        printf("unsupported security option\n");
        return STATE_PORT_TLS_INVALID_CRED_OPTION;
    }

    res = mbedtls_ssl_setup(&network_handle->mbedtls.ssl_ctx, &network_handle->mbedtls.ssl_config);
    if (res < 0) {
        printf("mbedtls_ssl_setup error, res: -0x%04X\n", -res);
        return res;
    }

    if (_host_is_ip(network_handle->host) == 0) {
        res = mbedtls_ssl_set_hostname(&network_handle->mbedtls.ssl_ctx, network_handle->host);
        if (res < 0) {
            printf("mbedtls_ssl_set_hostname error, res: -0x%04X\n", -res);
            return res;
        }
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        mbedtls_ssl_set_timer_cb(&network_handle->mbedtls.ssl_ctx, (void *)&network_handle->mbedtls.timer_delay_ctx,
                                 mbedtls_timing_set_delay, mbedtls_timing_get_delay);
    }

    /* UDP需要更具设备实际平台重新适配timing_delay回调 */
    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        mbedtls_ssl_set_timer_cb(&network_handle->mbedtls.ssl_ctx, (void *)&network_handle->mbedtls.timer_delay_ctx,
                                 mbedtls_timing_set_delay, mbedtls_timing_get_delay);
    }
    /* 配置mbedtls网络IO, 此处使用基于AT命令的网络接口 */
    mbedtls_ssl_set_bio(&network_handle->mbedtls.ssl_ctx, &network_handle->mbedtls.net_ctx, mbedtls_net_send_at,
                        mbedtls_net_recv_at, mbedtls_net_recv_timeout_at);
    mbedtls_ssl_conf_read_timeout(&network_handle->mbedtls.ssl_config, network_handle->connect_timeout_ms);

    while ((res = mbedtls_ssl_handshake(&network_handle->mbedtls.ssl_ctx)) != 0) {
        if ((res != MBEDTLS_ERR_SSL_WANT_READ) && (res != MBEDTLS_ERR_SSL_WANT_WRITE)) {
            printf("mbedtls_ssl_handshake error, res: -0x%04X\n", -res);
            if (res == MBEDTLS_ERR_SSL_INVALID_RECORD) {
                res = STATE_PORT_TLS_INVALID_RECORD;
            } else {
                res = STATE_PORT_TLS_INVALID_HANDSHAKE;
            }
            return res;
        }
    }

    res = mbedtls_ssl_get_verify_result(&network_handle->mbedtls.ssl_ctx);
    if (res < 0) {
        printf("mbedtls_ssl_get_verify_result error, res: -0x%04X\n", -res);
        return res;
    }

    printf("success to establish mbedtls connection, fd = %d(cost %d bytes in total, max used %d bytes)\n",
           (int)network_handle->mbedtls.net_ctx.fd,
           g_mbedtls_total_mem_used, g_mbedtls_max_mem_used);

    return 0;
}

static int32_t _core_sysdep_network_mbedtls_recv(core_network_handle_t *network_handle, uint8_t *buffer, uint32_t len,
        uint32_t timeout_ms)
{
    int res = 0;
    int32_t recv_bytes = 0;

    mbedtls_ssl_conf_read_timeout(&network_handle->mbedtls.ssl_config, timeout_ms);
    do {
        res = mbedtls_ssl_read(&network_handle->mbedtls.ssl_ctx, buffer + recv_bytes, len - recv_bytes);
        if (res < 0) {
            if (res == MBEDTLS_ERR_SSL_TIMEOUT) {
                break;
            } else if (res != MBEDTLS_ERR_SSL_WANT_READ &&
                       res != MBEDTLS_ERR_SSL_WANT_WRITE &&
                       res != MBEDTLS_ERR_SSL_CLIENT_RECONNECT) {
                if (recv_bytes == 0) {
                    printf("mbedtls_ssl_recv error, res: -0x%04X\n", -res);
                    if (res == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                        return STATE_PORT_TLS_RECV_CONNECTION_CLOSED;
                    } else if (res == MBEDTLS_ERR_SSL_INVALID_RECORD) {
                        return STATE_PORT_TLS_INVALID_RECORD;
                    } else {
                        return STATE_PORT_TLS_RECV_FAILED;
                    }
                }
                break;
            }
        } else if (res == 0) {
            break;
        } else {
            recv_bytes += res;
        }
    } while (recv_bytes < len);

    return recv_bytes;
}

int32_t _core_sysdep_network_mbedtls_send(core_network_handle_t *network_handle, uint8_t *buffer, uint32_t len,
        uint32_t timeout_ms)
{
    int32_t res = 0;
    int32_t send_bytes = 0;
    uint64_t timestart_ms = 0, timenow_ms = 0;
    struct timeval timestart, timenow;//, timeout;
#if 0
    /* timeout */
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
#endif

    /* Start Time */
    gettimeofday(&timestart, NULL);
    timestart_ms = timestart.tv_sec * 1000 + timestart.tv_usec / 1000;
    timenow_ms = timestart_ms;

    do {
        gettimeofday(&timenow, NULL);
        timenow_ms = timenow.tv_sec * 1000 + timenow.tv_usec / 1000;

        if (timenow_ms - timestart_ms >= timenow_ms ||
                timeout_ms - (timenow_ms - timestart_ms) > timeout_ms) {
            break;
        }

        res = mbedtls_ssl_write(&network_handle->mbedtls.ssl_ctx, buffer + send_bytes, len - send_bytes);
        if (res < 0) {
            if (res != MBEDTLS_ERR_SSL_WANT_READ &&
                    res != MBEDTLS_ERR_SSL_WANT_WRITE) {
                if (send_bytes == 0) {
                    printf("mbedtls_ssl_send error, res: -0x%04X\n", -res);
                    if (res == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                        return STATE_PORT_TLS_SEND_CONNECTION_CLOSED;
                    } else if (res == MBEDTLS_ERR_SSL_INVALID_RECORD) {
                        return STATE_PORT_TLS_INVALID_RECORD;
                    } else {
                        return STATE_PORT_TLS_SEND_FAILED;
                    }
                }
                break;
            }
        } else if (res == 0) {
            break;
        } else {
            send_bytes += res;
        }
    } while (((timenow_ms - timestart_ms) < timeout_ms) && (send_bytes < len));

    return send_bytes;
}

static void _core_sysdep_network_mbedtls_disconnect(core_network_handle_t *network_handle)
{
    mbedtls_ssl_close_notify(&network_handle->mbedtls.ssl_ctx);
    mbedtls_net_free(&network_handle->mbedtls.net_ctx);
    if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA) {
        mbedtls_x509_crt_free(&network_handle->mbedtls.x509_server_cert);
        mbedtls_x509_crt_free(&network_handle->mbedtls.x509_client_cert);
        mbedtls_pk_free(&network_handle->mbedtls.x509_client_pk);
    }
    mbedtls_ssl_free(&network_handle->mbedtls.ssl_ctx);
    mbedtls_ssl_config_free(&network_handle->mbedtls.ssl_config);
    g_mbedtls_total_mem_used = g_mbedtls_max_mem_used = 0;
}
#endif /* #ifdef CORE_SYSDEP_MBEDTLS_ENABLED */

void *core_sysdep_malloc(uint32_t size, char *name)
{
    void * result = malloc(size);
    if(NULL == result) {
        printf("failed to malloc: %u\n",size);
    }
    return result;
}

void core_sysdep_free(void *ptr)
{
    free(ptr);
}

volatile uint64_t client_server_detal_time = 0;
uint64_t core_sysdep_time(void)
{
    return (uint64_t)(osKernelGetTickCount() + client_server_detal_time);
    // return (uint64_t)(xTaskGetTickCount() * 1000)/configTICK_RATE_HZ;
}

void core_sysdep_sleep(uint64_t time_ms)
{
    osDelay(time_ms);
}

void *core_sysdep_network_init(void)
{
    core_network_handle_t *network_handle = malloc(sizeof(core_network_handle_t));
    if (network_handle == NULL) {
        return NULL;
    }
    memset(network_handle, 0, sizeof(core_network_handle_t));
    if (aiot_at_nwk_open(&network_handle->fd) < 0) {
        free(network_handle);
        return NULL;
    }

    return network_handle;
}

int32_t core_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == NULL || data == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    if (option >= CORE_SYSDEP_NETWORK_MAX) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    switch (option) {
    case CORE_SYSDEP_NETWORK_SOCKET_TYPE: {
        network_handle->socket_type = *(core_sysdep_socket_type_t *)data;
    }
    break;
    case CORE_SYSDEP_NETWORK_HOST: {
        network_handle->host = malloc(strlen(data) + 1);
        if (network_handle->host == NULL) {
            printf("malloc failed\n");
            return STATE_PORT_MALLOC_FAILED;
        }
        memset(network_handle->host, 0, strlen(data) + 1);
        memcpy(network_handle->host, data, strlen(data));
    }
    break;
    case CORE_SYSDEP_NETWORK_PORT: {
        network_handle->port = *(uint16_t *)data;
    }
    break;
    case CORE_SYSDEP_NETWORK_CONNECT_TIMEOUT_MS: {
        network_handle->connect_timeout_ms = *(uint32_t *)data;
    }
    break;
#ifdef CORE_SYSDEP_MBEDTLS_ENABLED
    case CORE_SYSDEP_NETWORK_CRED: {
        network_handle->cred = malloc(sizeof(aiot_sysdep_network_cred_t));
        if (network_handle->cred == NULL) {
            printf("malloc failed\n");
            return STATE_PORT_MALLOC_FAILED;
        }
        memset(network_handle->cred, 0, sizeof(aiot_sysdep_network_cred_t));
        memcpy(network_handle->cred, data, sizeof(aiot_sysdep_network_cred_t));
    }
    break;
    case CORE_SYSDEP_NETWORK_PSK: {
        core_sysdep_psk_t *psk = (core_sysdep_psk_t *)data;
        network_handle->psk.psk_id = malloc(strlen(psk->psk_id) + 1);
        if (network_handle->psk.psk_id == NULL) {
            printf("malloc failed\n");
            return STATE_PORT_MALLOC_FAILED;
        }
        memset(network_handle->psk.psk_id, 0, strlen(psk->psk_id) + 1);
        memcpy(network_handle->psk.psk_id, psk->psk_id, strlen(psk->psk_id));
        network_handle->psk.psk = malloc(strlen(psk->psk) + 1);
        if (network_handle->psk.psk == NULL) {
            free(network_handle->psk.psk_id);
            printf("malloc failed\n");
            return STATE_PORT_MALLOC_FAILED;
        }
        memset(network_handle->psk.psk, 0, strlen(psk->psk) + 1);
        memcpy(network_handle->psk.psk, psk->psk, strlen(psk->psk));
    }
    break;
#endif
    default: {
        printf("unknown option, %d\n", option);
    }
    }

    return STATE_SUCCESS;
}

static int32_t _core_sysdep_network_tcp_establish(core_network_handle_t *network_handle)
{
    return aiot_at_nwk_connect(network_handle->fd, network_handle->host, network_handle->port, network_handle->connect_timeout_ms);
}

int32_t core_sysdep_network_establish(void *handle)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        if (network_handle->host == NULL) {
            return STATE_PORT_MISSING_HOST;
        }
        if (network_handle->cred == NULL) {
            return _core_sysdep_network_tcp_establish(network_handle);
        } else {
            if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_NONE) {
                return _core_sysdep_network_tcp_establish(network_handle);
            }
#ifdef CORE_SYSDEP_MBEDTLS_ENABLED
            else {
                return _core_sysdep_network_mbedtls_establish(network_handle);
            }
#endif
        }

    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_SERVER) {
        return STATE_PORT_TCP_SERVER_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        return STATE_PORT_UDP_CLIENT_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_SERVER) {
        return STATE_PORT_UDP_SERVER_NOT_IMPLEMENT;
    }

    printf("unknown nwk type or tcp host absent\n");

    return STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE;
}

static int32_t _core_sysdep_network_tcp_recv(core_network_handle_t *network_handle, uint8_t *buffer, uint32_t len,
        uint32_t timeout_ms)
{
    return aiot_at_nwk_recv(network_handle->fd, buffer, len, timeout_ms);
}

int32_t core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                 core_sysdep_addr_t *addr)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == NULL || buffer == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    if (len == 0 || timeout_ms == 0) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        if (network_handle->cred == NULL) {
            return _core_sysdep_network_tcp_recv(network_handle, buffer, len, timeout_ms);
        } else {
            if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_NONE) {
                return _core_sysdep_network_tcp_recv(network_handle, buffer, len, timeout_ms);
            }
#ifdef CORE_SYSDEP_MBEDTLS_ENABLED
            else {
                return _core_sysdep_network_mbedtls_recv(network_handle, buffer, len, timeout_ms);
            }
#endif
        }
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_SERVER) {
        return STATE_PORT_TCP_SERVER_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        return STATE_PORT_UDP_CLIENT_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_SERVER) {
        return STATE_PORT_UDP_SERVER_NOT_IMPLEMENT;
    }

    return STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE;
}

int32_t _core_sysdep_network_tcp_send(core_network_handle_t *network_handle, uint8_t *buffer, uint32_t len,
                                      uint32_t timeout_ms)
{
    return aiot_at_nwk_send(network_handle->fd, buffer, len, timeout_ms);
}

int32_t core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                 core_sysdep_addr_t *addr)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == NULL || buffer == NULL) {
        printf("invalid parameter\n");
        return STATE_PORT_INPUT_NULL_POINTER;
    }
    if (len == 0 || timeout_ms == 0) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        if (network_handle->cred == NULL) {
            return _core_sysdep_network_tcp_send(network_handle, buffer, len, timeout_ms);
        } else {
            if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_NONE) {
                return _core_sysdep_network_tcp_send(network_handle, buffer, len, timeout_ms);
            }
#ifdef CORE_SYSDEP_MBEDTLS_ENABLED
            else {
                return _core_sysdep_network_mbedtls_send(network_handle, buffer, len, timeout_ms);
            }
#endif
        }
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_SERVER) {
        return STATE_PORT_TCP_SERVER_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        return STATE_PORT_UDP_CLIENT_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_SERVER) {
        return STATE_PORT_UDP_SERVER_NOT_IMPLEMENT;
    }

    return STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE;
}

static void _core_sysdep_network_tcp_disconnect(core_network_handle_t *network_handle)
{
    aiot_at_nwk_close(network_handle->fd);
}

int32_t core_sysdep_network_deinit(void **handle)
{
    core_network_handle_t *network_handle = *(core_network_handle_t **)handle;

    if (handle == NULL || *handle == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    /* Shutdown both send and receive operations. */
    if (network_handle->socket_type == 0 && network_handle->host != NULL) {
        if (network_handle->cred == NULL) {
            _core_sysdep_network_tcp_disconnect(network_handle);
        } else {
            if (network_handle->cred->option == AIOT_SYSDEP_NETWORK_CRED_NONE) {
                _core_sysdep_network_tcp_disconnect(network_handle);
            }
#ifdef CORE_SYSDEP_MBEDTLS_ENABLED
            else {
                _core_sysdep_network_mbedtls_disconnect(network_handle);
            }
#endif
        }
    }

    if (network_handle->host != NULL) {
        free(network_handle->host);
        network_handle->host = NULL;
    }
    if (network_handle->cred != NULL) {
        free(network_handle->cred);
        network_handle->cred = NULL;
    }
#ifdef CORE_SYSDEP_MBEDTLS_ENABLED
    if (network_handle->psk.psk_id != NULL) {
        free(network_handle->psk.psk_id);
        network_handle->psk.psk_id = NULL;
    }
    if (network_handle->psk.psk != NULL) {
        free(network_handle->psk.psk);
        network_handle->psk.psk = NULL;
    }
#endif

    free(network_handle);
    *handle = NULL;

    return 0;
}

void core_sysdep_rand(uint8_t *output, uint32_t output_len)
{
		uint32_t idx = 0, bytes = 0, rand_num = 0;

		srand(core_sysdep_time());


    for (idx = 0; idx < output_len;) {
        if (output_len - idx < 4) {
            bytes = output_len - idx;
        } else {
            bytes = 4;
        }
        rand_num = rand();
        while (bytes-- > 0) {
            output[idx++] = (uint8_t)(rand_num >> bytes * 8);
        }
    }
}

void *core_sysdep_mutex_init(void)
{
    return (void *)osMutexNew(NULL);
}

void core_sysdep_mutex_lock(void *mutex)
{
    osMutexAcquire((osMutexId_t)mutex, osWaitForever);
}

void core_sysdep_mutex_unlock(void *mutex)
{
    osMutexRelease((osMutexId_t)mutex);
}

void core_sysdep_mutex_deinit(void **mutex)
{
    if (mutex == NULL || *mutex == NULL) {
       return;
    }
    osMutexDelete((osMutexId_t)*mutex);
}

aiot_sysdep_portfile_t g_aiot_sysdep_portfile = {
    core_sysdep_malloc,
    core_sysdep_free,
    core_sysdep_time,
    core_sysdep_sleep,
    core_sysdep_network_init,
    core_sysdep_network_setopt,
    core_sysdep_network_establish,
    core_sysdep_network_recv,
    core_sysdep_network_send,
    core_sysdep_network_deinit,
    core_sysdep_rand,
    core_sysdep_mutex_init,
    core_sysdep_mutex_lock,
    core_sysdep_mutex_unlock,
    core_sysdep_mutex_deinit
};

