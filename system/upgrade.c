#include "upgrade.h"
#include "sflash.h"
#include "dal_nor.h"
#include "sflash.h"
#include "md5.h"
#include "dal.h"
#include "log.h"
#include "cfg.h"


#define FW_LEN                  (4*KB)
#define TMP_LEN                 (10*KB)

typedef enum {
    FW_SRC_CUR=0,
    FW_SRC_FAC,
    FW_SRC_TMP
    
}FW_SRC;



static int upg_init_flag=0;
static int get_upg_info(upg_info_t *info);
static md5_ctx_t md5Ctx;
static int fw_invalid=0;
static void print_upg_info(char *s, upg_info_t *info)
{
    LOGD("\n");
    LOGD("__%s__upgInfo->head: 0x%08x\n",      s, info->head);
    LOGD("__%s__upgInfo->tail: 0x%08x\n",      s, info->tail);
    LOGD("__%s__upgInfo->fwAddr: 0x%08x\n",    s, info->fwAddr);
    LOGD("__%s__upgInfo->runAddr: 0x%08x\n",   s, info->runAddr);
    LOGD("__%s__upgInfo->upgCnt: %d\n",        s, info->upgCnt);
    LOGD("__%s__upgInfo->upgFlag: %d\n",       s, info->upgFlag);
    LOGD("__%s__upgInfo->facFillFlag: %d\n",   s, info->facFillFlag);
    LOGD("\n");
}

static void print_fw_info(char *s, fw_info_t *info)
{
    LOGD("\n");
    LOGD("__%s__fwInfo->magic:   0x%08x\n",    s, info->magic);
    LOGD("__%s__fwInfo->version: %s\n",        s, info->version);
    LOGD("__%s__fwInfo->bldtime: %s\n",        s, info->bldtime);
    LOGD("__%s__fwInfo->length:  %d\n",        s, info->length);
    LOGD("\n");
}
static void print_jump_info(char *s, upg_info_t *info)
{
    U32 addr[2];
    dal_nor_read(info->runAddr, addr, 8);
    
    LOGD("__%s jump addr: 0x%08x, 0x%08x\n", s, addr[0], addr[1]);
}

#define CHAR(b) (((b)<=9)?((b)+0x30):((b)-10+0x61));
static void byte2char(U8 *chr, U8 b)
{
    chr[0] = CHAR(b>>4);
    chr[1] = CHAR(b&0x0f);
}
static void print_md5(char *s, md5_t *m)
{
    U8 i;
    
    LOGD("__%s", s);
    for(i=0; i<32; i++) {
        LOGD("%c", m->digit[i]);
    }
    LOGD("\n");
}


static int upg_info_valid(upg_info_t *info)
{
    int r;
    mcu_info_t mInfo;
    U32 flashStart,flashEnd;

    r = dal_get_info(&mInfo);
    if(r) {
        LOGE("__upg_info_valid, dal_get_info failed!\n");
        return 0;
    }
    
    if(info->head!=UPG_HEAD_MAGIC) {
        LOGE("__upg_info_valid, upg info.head: 0x%08x is wrong, correct is: 0x%08x\n", info->head, UPG_HEAD_MAGIC);
        return 0;
    }
    
    if(info->tail!=UPG_TAIL_MAGIC) {
        LOGE("__upg_info_valid, upg info.tail: 0x%08x is wrong, correct is: 0x%08x\n", info->head, UPG_TAIL_MAGIC);
        return 0;
    }

    if(info->upgFlag>=UPG_FLAG_MAX) {
        LOGE("__upg_info_valid, upg info.upgFlag: %d is wrong, correct is: 0~%d\n", info->upgFlag, UPG_FLAG_MAX);
        return 0;
    }
    
    if(info->facFillFlag>1) {
        LOGE("__upg_info_valid, upg info.facFillFlag: %d is wrong, correct is: 0 or 1\n", info->facFillFlag);
        return 0;
    }

    if((info->fwAddr<mInfo.flashStart) || (info->fwAddr>mInfo.flashEnd)) {
        LOGE("__upg_info_valid, upg info.fwAddr: 0x%08x is wrong, correct is: 0x%08x~0x%08x\n", info->fwAddr, mInfo.flashStart, mInfo.flashEnd);
        return 0;
    }
    
    return 1;
}
static int fw_info_valid(fw_info_t *info)
{
    int r=1;
    char *bt1="20000101 00:00:00";
    char *bt2="20991231 23:59:59";
    
    if((info->magic!=FW_MAGIC)) {
        LOGE("__fw_info_valid, magic: 0x%08x is wrong, correct is: 0x%08x\n", info->magic, FW_MAGIC);
        return 0;
        
    }
        
    if((info->length<10*KB) || (info->length>1*MB)) {
        LOGE("__fw_info_valid, length: %d is wrong, correct is: %d~%d\n", info->length, 10*KB, 1*MB);
        return 0;
    }
    
    if(strcmp((const char*)info->bldtime, bt1)<0 || strcmp((const char*)info->bldtime, bt2)>0) {
        LOGE("__fw_info_valid, bldtime: %s is wrong, correct is: %s~%s\n", info->bldtime, bt1, bt2);
        return 0;
    }
    
    return 1;
}

//src, 0: nor, 1:sflash
static int fw_is_valid(U32 addr, int src)
{
    typedef int (*fw_read_t)(U32 addr, void *data, U32 len);
    U8 i,tmp[16];
    int r,rlen,tlen=0;
    fw_info_t fwInfo;
    md5_t m1,m2;
    U32 xaddr=addr;
    U8 *pbuf=NULL;
    fw_read_t read_fn;
    
    read_fn = (src==0)?dal_nor_read:sflash_read;
    
    r = read_fn(xaddr, &fwInfo, sizeof(fw_info_t));
    if(r) {
        LOGE("__fw_is_valid, %s failed\n", src?"sflash_read":"dal_nor_read");
        return 0;
    }
    if(!fw_info_valid(&fwInfo)) {
        LOGE("__fw_is_valid, fw info is invalid!\n");
        return 0;
    }
    print_fw_info(">>>", &fwInfo);
    
    pbuf = (U8*)malloc(FW_LEN);
    if(!pbuf) {
        LOGE("__fw_is_valid, malloc falied\n");
        return 0;
    }
    
    md5_init(&md5Ctx);
    
    while(1) {
        if(tlen+FW_LEN<fwInfo.length) {
            rlen = FW_LEN;
        }
        else {
            rlen = fwInfo.length-tlen;
        }
        
        r = read_fn(xaddr+tlen, pbuf, rlen);
        if(r) {
            LOGE("__fw_is_valid, read_fn failed\n");
            break;
        }
        tlen += rlen;
        
        md5_update(&md5Ctx, pbuf, rlen);
        
        if(tlen>=fwInfo.length) {
            break;
        }
    }
    
    if(r==0) {
        LOGD("__fw_is_valid, read md5 from 0x%x, %d\n", xaddr, fwInfo.length);
        read_fn(xaddr+fwInfo.length, &m1, sizeof(m1));
    
        md5_final(&md5Ctx, tmp);
        for (i=0; i<16; i++) {
            byte2char(&m2.digit[i*2], tmp[i]);
        }
        
        print_md5("ori-md5: ", &m1);
        print_md5("cal-md5: ", &m2);
        
        if(memcmp(&m1, &m2, sizeof(md5_t))) {
            r = -1;
        }
    }
    free(pbuf);
    
    return (r==0)?1:0;
}



static int get_fw_info(U32 fwAddr, fw_info_t *info)
{
    int r;
    U32 realAddr=fwAddr;
    upg_info_t upgInfo;
    
    if(fwAddr==0) {
        r = get_upg_info(&upgInfo);
        if(r) {
            LOGE("__get_fw_info, get_upg_info failed\n");
            return -1;
        }
        
        realAddr = upgInfo.fwAddr;
    }
    
    //LOGD("__get_fw_info, realAddr: 0x%x\n", realAddr);
    r = dal_nor_read(realAddr, info, sizeof(fw_info_t));
    if(r) {
        LOGE("__get_fw_info, dal_nor_read failed\n");
        return -1;
    }
    
    if(!fw_info_valid(info)) {
        LOGE("__get_fw_info, fw info is invalid\n");
        return -1;
    }
    
    return 0;
}
static int get_upg_fw_info(U32 addr, fw_info_t *info)
{
    int r;
    
    r = sflash_read(addr, info, sizeof(fw_info_t));
    if(r) {
        LOGE("__get_upg_fw_info, sflash_read failed\n");
        return -1;
    }
    
    if(!fw_info_valid(info)) {
        LOGE("__get_upg_fw_info, fw info is invalid\n");
        return -1;
    }
    
    return 0;
}
static int get_upg_info(upg_info_t *info)
{
    int r;
    
    r = dal_nor_read(UPG_INFO_OFFSET, (U8*)info, sizeof(upg_info_t));
    if(r) {
        LOGE("__get_upg_info, dal_nor_read failed\n");
        return -1;
    }
    
    if(!upg_info_valid(info)) {
        LOGE("__get_upg_info, upg info is invalid\n");
        return -1;
    }
    
    return 0;
}
static int set_upg_info(upg_info_t *info)
{
    int r;
    U32 addr=UPG_INFO_OFFSET;
    upg_info_t info2;
    
    dal_nor_erase(addr, sizeof(upg_info_t));
    
    r = dal_nor_write(addr, info, sizeof(upg_info_t), 1);
    if(r) {
        LOGE("__set_upg_info, dal_nor_write failed\n");
        return -1;
    }
     
    return 0;
}
static int upg_info_init(upg_info_t *info)
{
    info->fwAddr  = APP_OFFSET;
    info->runAddr = info->fwAddr+sizeof(upg_info_t);
    info->facFillFlag = 0;
    info->head = UPG_HEAD_MAGIC;
    info->tail = UPG_TAIL_MAGIC;
    info->upgFlag = UPG_FLAG_NORMAL;
    info->upgCnt = 0;
    
    return set_upg_info(info);
}



static int upg_set_flag(U32 flag)
{
    int r;
    upg_info_t info;
    
    r = get_upg_info(&info);
    if(r) {
        LOGE("__upg_set_flag, get_upg_info 1 failed!\n");
        return -1;
    }
    
    info.upgFlag = flag;
    if(flag==UPG_FLAG_FINISH) {
        info.upgCnt++;
    }
    
    r = set_upg_info(&info);
    if(r) {
        LOGE("__upg_set_flag, set_upg_info failed!\n");
        return -1;
    }
    
    return 0;
}
static int upg_set_fill_flag(U32 flag)
{
    int r;
    upg_info_t info;
    
    r = get_upg_info(&info);
    if(r) {
        LOGE("__upg_set_fill_flag, get_upg_info 1 failed!\n");
        return -1;
    }
    
    info.facFillFlag = flag;
    
    r = set_upg_info(&info);
    if(r) {
        LOGE("__upg_set_fill_flag, set_upg_info 1 failed!\n");
        return -1;
    }
    LOGD("__upg_set_fill_flag %d ok!\n", flag);
    
    return 0;
}


static int upg_fw_copy(upg_info_t *info, FW_SRC src)
{
    int r=0;
    char tmp[20];
    U8 *pbuf=malloc(FW_LEN);
    U32 tlen=0,rlen,fwlen;
    U32 srcAddr,dstAddr;
    fw_info_t fwInfo;
    
    if(!pbuf) {
        LOGE("__upg_fw_copy, malloc failed\n");
        return -1;
    }
    
    srcAddr = (src==FW_SRC_TMP)?TMP_APP_OFFSET:FAC_APP_OFFSET;
    dstAddr = info->fwAddr;
    
    if(!fw_is_valid(srcAddr, 1)) {
        LOGE("__upg_fw_copy, %s fw is invalid!\n", (src==FW_SRC_TMP)?"tmp":"fac");
        return -1;
    }
    
    r = get_upg_fw_info(srcAddr, &fwInfo);
    if(r) {
        LOGE("__upg_fw_copy, get_fw_info failed\n");
        free(pbuf); return -1;
    }
    print_fw_info((src==FW_SRC_TMP)?"tmp":"fac", &fwInfo);
    
    if(!fw_info_valid(&fwInfo)) {
        LOGE("__upg_fw_copy, fw info is invalid!\n");
        return -1;
    }
    fwlen = fwInfo.length+sizeof(md5_t);
    
    LOGD("__upg_fw_copy, from 0x%x(sflash) to 0x%x(norflash), len: %d\n", srcAddr, dstAddr, fwlen);
    
    r = dal_nor_erase(dstAddr, fwlen);
    if(r) {
        LOGE("__upg_fw_copy, dal_nor_erase failed!\n");
        return -1;
    }
    
    while(tlen<fwlen) {
        if(tlen+FW_LEN<fwlen) {
            rlen = FW_LEN;
        }
        else {
            rlen = fwlen-tlen;
        }
        
        sflash_read(srcAddr, pbuf, rlen);
        r = dal_nor_write(dstAddr, pbuf, rlen, 1);
        if(r==0) {
            srcAddr += rlen; dstAddr += rlen; tlen += rlen;
            LOGD("__upg_fw_copy total len: %d\n", tlen);
        }
        else {
            LOGE("__upg_fw_copy failed, addr: 0x%x, len: %d\n", dstAddr, rlen);
            break;
        }
    }
    LOGD("__upg_fw_copy finished\n");
    free(pbuf);
    
    
    if(r==0 && tlen>sizeof(fwInfo)) {
        r = get_fw_info(0, &fwInfo);
        print_fw_info("%%%", &fwInfo);
        print_jump_info("%%%", info);
    }
    
    if(r==0) {
        r = upg_set_flag(UPG_FLAG_FINISH);
    }
    
    return r;
}


static int upg_fac_fill(upg_info_t *info)
{
    int r=0;
    U8 *pbuf=malloc(FW_LEN);
    U32 tlen=0,rlen,fwlen;
    U32 srcAddr,dstAddr;
    fw_info_t fwInfo;
    
    if(pbuf) {
        srcAddr = info->fwAddr;
        dstAddr = FAC_APP_OFFSET;
        
        if(!fw_is_valid(srcAddr, 0)) {
            LOGE("__upg_fac_fill, current fw is invalid!\n");
            return -1;
        }
        
        r = dal_nor_read(srcAddr, &fwInfo, sizeof(fwInfo));
        if(r) {
            LOGE("__upg_fac_fill, read fw info failed\n");
            free(pbuf);
            return -1;
        }
        fwlen = fwInfo.length+sizeof(md5_t);
        
        while(tlen<fwlen) {
            if(tlen+FW_LEN<fwlen) {
                rlen = FW_LEN;
            }
            else {
                rlen = fwlen-tlen;
            }
            
            dal_nor_read(srcAddr+tlen, pbuf, rlen);
            r = sflash_write(dstAddr+tlen, pbuf, rlen, 1);
            if(r) {
                LOGE("__upg_fill_fac failed addr: 0x%x, len: %d\n", dstAddr, tlen);
                break;
            }
            tlen += rlen;
        }
        free(pbuf);
        
        if(r==0) {
            LOGD("__upg_fill_fac, upg_set_fill_flag\n");
            r = upg_set_fill_flag(1);
        }
    }
    
    return r;
}

/////////////////////////////////////////////////////////////////
int upgrade_init(void)
{
    int r=0;

    if(!upg_init_flag) {
        r = sflash_init();
        if(r) {
           return -1; 
        }
        
        r = dal_nor_init();
        if(r==0) {
            upg_init_flag = 1;
            LOGD("__upgrade_init ok!\n");
        }
    }
    
    return r;
}


int upgrade_deinit(void)
{
    int r;
    
    r = sflash_deinit();
    r |= dal_nor_deinit();
    
    return r;
}


int upgrade_check(U32 *runAddr)
{
    int r,r1,fac_flag=0;
    fw_info_t  fwInfo;
    upg_info_t upgInfo;
    
    upgrade_init();
    
    r = get_upg_info(&upgInfo);
    if(r) {
        return -1;
    }
    r = get_fw_info(upgInfo.fwAddr, &fwInfo);
    if(r) {
        return -1;
    }
    
#ifdef BOOTLOADER
    print_upg_info("boot upg", &upgInfo);
    print_fw_info("boot fw", &fwInfo);
    
    //if upgrage info not be inited, init it to the default value
    if(!upg_info_valid(&upgInfo)) {
        LOGD("__invalid upg info data, init it now ...\n");
        
        r = upg_info_init(&upgInfo);
        if(r) {
            LOGE("__upg_info_init failed!\n");
        }
        else {
            LOGD("__upg_info_init ok!\n");
        }
    }
    
    if(upgInfo.facFillFlag==0) {
        r1 = upg_fac_fill(&upgInfo);
        if(r1) {
            LOGE("__upg_fill_fac failed!\n");
        }
        else {
            LOGD("__upg_fill_fac ok!\n");
        }
    }
    
    if(upgInfo.upgFlag==UPG_FLAG_START) {
        LOGD("__upgrading now !!! copy tmp fw to the run fw area ...\n");
        
        r1 = upg_fw_copy(&upgInfo, FW_SRC_TMP);         //copy tmp fw to cur fw
        if(r1) {
            LOGE("__upg_copy_tmp_fw failed, set upagrade flag to normal!\n");
            upg_set_flag(UPG_FLAG_NORMAL);
        }
        else {
            LOGD("__upg_copy_tmp_fw ok!\n");
        }
    }
    
    if(!fw_is_valid(upgInfo.fwAddr, 0)) {
        fac_flag = 1;
    }
    
    if(fac_flag>0) {
        LOGD("__back to default fw, copy fac fw to the run fw area ...\n");
        
        r1 = upg_fw_copy(&upgInfo, FW_SRC_TMP);         //copy fac fw to cur fw
        if(r1) {
            LOGE("__upg_copy_fac_fw failed!\n");
        }
        else {
            LOGD("__upg_copy_fac_fw ok!\n");
        }
    }
    
#else
    print_upg_info("app upg", &upgInfo);
    print_fw_info("app fw", &fwInfo);
    
    if(upgInfo.upgFlag==UPG_FLAG_FINISH) {
        r = upg_set_flag(UPG_FLAG_NORMAL);
        LOGD("__upg_set_normal_flag: %d\n", r);
    }
#endif
    
    if(runAddr) {
        LOGD("___upgInfo.runAddr: 0x%08x\n", upgInfo.runAddr);
        *runAddr = upgInfo.runAddr;
    }
    
    return r;
}


int upgrade_get_fw_info(fw_info_t *info)
{
    if(!info) {
        return -1;
    }
    
    return get_fw_info(0, info);
}


int upgrade_get_upg_info(upg_info_t *info)
{
    int r;
    
    if(!info) {
        return -1;
    }
    
    r = get_upg_info(info);
    if(r) {
        return -1;
    }
    
    return -2;
}


int upgrade_erase(int total_len)
{
    return sflash_erase(TMP_APP_OFFSET, total_len);
}



int upgrade_write(U8 *data, int len, int index)
{
    int r;
    U32 addr=TMP_APP_OFFSET;
    static int upg_tlen=0;
    fw_info_t fwInfo={0};
    
    if(!data || len<=0) {
        return -1;
    }
    
    if(index==0) {
        upg_tlen = 0;
    }
    
    r = sflash_write(addr+upg_tlen, data, len, 1);
    if(r) {
        upg_tlen = 0;
        LOGE("___upgrade_write 0x%x %d failed!\n", addr+upg_tlen, len);
        return -1;
    }
    
    upg_tlen += len;
    LOGD("___upgrade_write tlen: %d\n", upg_tlen);
    
    if(index==0) {
        sflash_read(addr, &fwInfo, sizeof(fwInfo));
        print_fw_info("$$$", &fwInfo);
    }
    else if(index<0){
        upg_tlen = 0;
        r = upg_set_flag(UPG_FLAG_START);
    }
    
    return 0;
}


int upgrade_fac_update(void)
{
    return upg_set_fill_flag(0);
}



int upgrade_handle(void *data, int len)
{
    int r=0;
    U8  err=0;
    static U8 *pbuf=NULL;
    file_hdr_t *hdr=(file_hdr_t*)data;

    #if 0
    if(!pbuf) {
        pbuf = malloc(TMP_LEN);
    }

    if(hdr->dataLen==0) {
        return ERROR_FW_PKT_LENGTH;
    }
    
    if(hdr->pkts==0 || (hdr->pid>0 && hdr->pkts>0 && hdr->pid>=hdr->pkts)) {
        return ERROR_FW_PKT_ID;
    }
    
    if(hdr->pid==0) {
        
        
    }
    
    if(hdr->pid!=upgCurPktId) {
        return ERROR_FW_PKT_ID;
    }
#endif
    
    
    return r;
}







