#ifndef __YAFFS_CONFIG_H__
#define __YAFFS_CONFIG_H__

#include "types.h"

#define CONFIG_YAFFS_DIRECT
#define CONFIG_YAFFS_YAFFS2
#define CONFIG_YAFFSFS_PROVIDE_VALUES
#define CONFIG_YAFFS_DEFINES_TYPES
#define CONFIG_YAFFS_PROVIDE_DEFS

typedef long          loff_t;
typedef long          off_t;
typedef unsigned long mode_t;
typedef unsigned long dev_t;


#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14


#include "ydirectenv.h"
#include "yportenv.h"


#endif

