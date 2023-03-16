#ifndef __WAV_Hx__
#define __WAV_Hx__

#include "types.h"

#ifdef __cplusplus
  extern "C" {
#endif

#define CHMAX       16
#define WAV_MAGIC   0xAABBEEFFU


#pragma pack (1)
typedef struct {
    U8 year;           // 0~99 -> 2000 ~ 2099, 7 bit
    U8 month;          // 1 ~ 12, 4 bit
    U8 day;            // 1 ~ 31, 5 bit
    U8 hour;           // 0 ~ 23, 5 bit
    U8 min;            // 0 ~ 59, 6 bit
    U8 sec;            // 0 ~ 59, 6 bit
    U16 msec;          // 0 ~ 999, 10 bit
} timestamp_t;

typedef struct {
    U32             len;   //wave_data_t total length, including the data length
    U8              ch;
    U32             sr;    //samplerate
    timestamp_t     ts;

    F32             data[0];
}wav_data_t;

#define WAV_TMP_MAX 2000
typedef struct {
  U32             len;   //wave_data_t total length, including the data length
  U8              ch;
  U32             sr;    //samplerate
  timestamp_t     ts;

  F32             data[TMP_MAX];
}wav_tmp_t;

typedef struct {
    U32             magic;
    U8              mode;
    U8              format;
    U32             len;
    wav_data_t      data[0];
}wav_pkt_t;
#pragma pack ()



wav_pkt_t* wav_pkt_init(U32 total_data_len);
int wav_pkt_deinit(wav_pkt_t *h);
int wav_pkt_pack(wav_pkt_t *p, U8 ch, F32 *data, U32 len);
int wav_pkt_unpack(wav_pkt_t *p, U8 idx, wav_data_t *wd);
#ifdef __cplusplus
  }
#endif

#endif

