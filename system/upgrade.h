#ifndef __UPGRADE_Hx__
#define __UPGRADE_Hx__

#include "types.h"
#include "protocol.h"

int upgrade_init(void);
int upgrade_deinit(void);
int upgrade_check(U32 *jumpAddr, U8 bBoot);

int upgrade_erase(int total_len);
int upgrade_info_erase(void);
int upgrade_write(U8 *data, int len, int index);
int upgrade_write_all(U8 *data, int len);

int upgrade_get_fw_info(fw_info_t *info);
int upgrade_get_upg_data(upg_data_t *upg);

int upgrade_fw_info_valid(fw_info_t *info);
int upgrade_read_fw(buf_t *buf);

#endif




