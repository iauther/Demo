#include "data.h"
#include "cfg.h"


const sett_t DEFAULT_SETT={
    .mode = 0,
};

const fw_info_t FW_INFO={
    .magic=FW_MAGIC,
    .version=VERSION,
    .bldtime=__DATE__,
};


sett_t curSett;


