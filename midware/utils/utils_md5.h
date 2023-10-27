/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */




#ifndef _IOTX_COMMON_MD5_H_
#define _IOTX_COMMON_MD5_H_

#include <stdint.h>


typedef struct {
    uint32_t total[2];          /*!< number of bytes processed  */
    uint32_t state[4];          /*!< intermediate digest state  */
    unsigned char buffer[64];   /*!< data block being processed */
} utils_md5_t;


#ifdef __cplusplus
extern "C" {
#endif


/**
 * \brief          Initialize MD5 context
 *
 * \param ctx      MD5 context to be initialized
 */
void utils_md5_init2(utils_md5_t *ctx);

/**
 * \brief          Clear MD5 context
 *
 * \param ctx      MD5 context to be cleared
 */
void utils_md5_free2(utils_md5_t *ctx);

/**
 * \brief          Clone (the state of) an MD5 context
 *
 * \param dst      The destination context
 * \param src      The context to be cloned
 */
void utils_md5_clone2(utils_md5_t *dst,
                     const utils_md5_t *src);

/**
 * \brief          MD5 context setup
 *
 * \param ctx      context to be initialized
 */
void utils_md5_start2(utils_md5_t *ctx);

/**
 * \brief          MD5 process buffer
 *
 * \param ctx      MD5 context
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 */
void utils_md5_update2(utils_md5_t *ctx, uint8_t *input, size_t ilen);

/**
 * \brief          MD5 final digest
 *
 * \param ctx      MD5 context
 * \param output   MD5 checksum result
 */
void utils_md5_finish2(utils_md5_t *ctx, uint8_t output[16]);

/* Internal use */
void utils_md5_process2(utils_md5_t *ctx, uint8_t data[64]);

/**
 * \brief          Output = MD5( input buffer )
 *
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 * \param output   MD5 checksum result
 */
void utils_md5(uint8_t *input, size_t ilen, uint8_t output[16]);


char utils_bin2hex(uint8_t hb);

void utils_md5_hexstr(uint8_t input[16], uint8_t output[32]);


#ifdef __cplusplus
}
#endif


#endif

