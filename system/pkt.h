#ifndef __PKT_Hx__
#define __PKT_Hx__

#include "types.h"
#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    PKT_TYPE_TRIGGER = 0,
    PKT_TYPE_LIGHT,

    PKT_TYPE_MAX
};

enum {
    CHK_NONE=0,
    CHK_SUM,
    CHK_CRC8,
    CHK_CRC16,
    CHK_CRC32,
};

typedef union {
    U8      sum;
    U16     crc8;
    U16     crc16;
    U32     crc32;
}chkcode_t;

typedef int (*pkt_callback_t)(ch_data_t *data);

int pkt_chk_len(int chkID);
int pkt_check_hdr(void* data, int dlen, int buflen, int chkID);
int pkt_pack_data(U8 type, U8 nAck, void* data, int dlen, U8 *buf, int blen, int chkID);
int pkt_pack_ack(U8 type, U8 err, U8* buf, int blen, int chkID);

#ifdef __cplusplus
    }
#endif
                            
#endif

