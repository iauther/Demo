#ifndef __UPGRADE_Hx__
#define __UPGRADE_Hx__

#include "types.h"
#include "protocol.h"

enum {
    FW_CUR=0,
    FW_FAC,
    FW_TMP,
    
    FW_MAX
};



int upgrade_init(void);
int upgrade_deinit(void);
int upgrade_proc(U32 *fwAddr, U8 bBoot);

int upgrade_erase(int total_len);
int upgrade_info_erase(void);
int upgrade_write(U8 *data, int len, int index);
int upgrade_write_all(U8 *data, int len);
int upgrade_read(U8 *data, int len);

int upgrade_get_fw_info(int fw, fw_hdr_t *hdr);

int upgrade_fw_info_valid(fw_info_t *info);
int upgrade_read_fw(buf_t *buf);
void upgrade_test(void);

#endif




