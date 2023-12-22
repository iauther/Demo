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

#define MALLOC   malloc
#define FREE     free


typedef enum {
    FW_SRC_CUR=0,
    FW_SRC_FAC,
    FW_SRC_TMP
}FW_SRC;



static int upg_init_flag=0;
static int get_upg_data(upg_data_t *upg);
static md5_ctx_t md5Ctx;
static void print_info(char *s, fw_info_t *fw, upg_data_t *upg)
{
    LOGD("\n");
    if(fw) {
        LOGD("__%s__fwInfo->magic:   0x%08x\n",    s, fw->magic);
        LOGD("__%s__fwInfo->version: %s\n",        s, fw->version);
        LOGD("__%s__fwInfo->bldtime: %s\n",        s, fw->bldtime);
        LOGD("__%s__fwInfo->length:  %d\n",        s, fw->length);
        LOGD("\n");
    }
    
    if(upg) {
        LOGD("__%s__upgInfo->head: 0x%08x\n",      s, upg->head);
        LOGD("__%s__upgInfo->tail: 0x%08x\n",      s, upg->tail);
        LOGD("__%s__upgInfo->fwAddr: 0x%08x\n",    s, upg->fwAddr);
        LOGD("__%s__upgInfo->runAddr: 0x%08x\n",   s, upg->runAddr);
        LOGD("__%s__upgInfo->upgCnt: %d\n",        s, upg->upgCnt);
        LOGD("__%s__upgInfo->upgFlag: %d\n",       s, upg->upgFlag);
        LOGD("__%s__upgInfo->facFillFlag: %d\n",   s, upg->facFillFlag);
        LOGD("\n");
    }
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


static int upg_data_valid(upg_data_t *upg)
{
    int r;
    mcu_info_t mInfo;

    r = dal_get_info(&mInfo);
    if(r) {
        LOGE("__upg_data_valid, dal_get_info failed!\n");
        return 0;
    }
    
    if(upg->head!=UPG_HEAD_MAGIC) {
        LOGE("__upg_data_valid, upg data.head: 0x%08x is wrong, correct is: 0x%08x\n", upg->head, UPG_HEAD_MAGIC);
        return 0;
    }
    
    if(upg->tail!=UPG_TAIL_MAGIC) {
        LOGE("__upg_data_valid, upg data.tail: 0x%08x is wrong, correct is: 0x%08x\n", upg->head, UPG_TAIL_MAGIC);
        return 0;
    }

    if(upg->upgFlag>=UPG_FLAG_MAX) {
        LOGE("__upg_data_valid, upg data.upgFlag: %d is wrong, correct is: 0~%d\n", upg->upgFlag, UPG_FLAG_MAX);
        return 0;
    }
    
    if(upg->facFillFlag>1) {
        LOGE("__upg_data_valid, upg data.facFillFlag: %d is wrong, correct is: 0 or 1\n", upg->facFillFlag);
        return 0;
    }

    if((upg->fwAddr<mInfo.flashStart) || (upg->fwAddr>mInfo.flashEnd)) {
        LOGE("__upg_data_valid, upg data.fwAddr: 0x%08x is wrong, correct is: 0x%08x~0x%08x\n", upg->fwAddr, mInfo.flashStart, mInfo.flashEnd);
        return 0;
    }
    
    return 1;
}
static int fw_info_valid(fw_info_t *info)
{
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
    print_info(src?"sf":"nor", &fwInfo, NULL);
    
    pbuf = (U8*)MALLOC(FW_LEN);
    if(!pbuf) {
        LOGE("__fw_is_valid, MALLOC falied\n");
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
    FREE(pbuf);
    
    return (r==0)?1:0;
}

/*
    fwAddr=0, 从upg升级信息区读固件信息
    fwAddr>0, 视该地址为片内flash地址，从该地址读取固件信息
*/
static int get_fw_info(U32 fwAddr, fw_info_t *info)
{
    int r;
    U32 realAddr=fwAddr;
    upg_data_t upg;
    upg_hdr_t  hdr;
    
    if(fwAddr==0) {
        r = get_upg_data(&upg);
        if(r) {
            LOGE("__get_fw_info, get_upg_data failed\n");
            return -1;
        }
        
        realAddr = upg.fwAddr;
    }
    
    //LOGD("__get_fw_info, realAddr: 0x%x\n", realAddr);
    r = dal_nor_read(realAddr, &hdr, sizeof(hdr));
    if(r) {
        LOGE("__get_fw_info, dal_nor_read failed\n");
        return -1;
    }
    
    if(!fw_info_valid(&hdr.fw)) {
        LOGE("__get_fw_info, fw info is invalid\n");
        return -1;
    }
    *info = hdr.fw;
    
    return 0;
}
static int get_upg_fw_info(U32 addr, fw_info_t *info)
{
    int r;
    upg_hdr_t  hdr;
    
    r = sflash_read(addr, &hdr, sizeof(hdr));
    if(r) {
        LOGE("__get_upg_fw_info, sflash_read failed\n");
        return -1;
    }
    
    if(!fw_info_valid(&hdr.fw)) {
        LOGE("__get_upg_fw_info, fw info is invalid\n");
        return -1;
    }
    *info = hdr.fw;
    
    return 0;
}
static int get_upg_data(upg_data_t *upg)
{
    int r;
    
    r = dal_nor_read(UPG_INFO_OFFSET, (U8*)upg, sizeof(upg_data_t));
    if(r) {
        LOGE("__get_upg_data, dal_nor_read failed\n");
        return -1;
    }
    
    if(!upg_data_valid(upg)) {
        LOGE("__get_upg_data, upg info is invalid\n");
        return -1;
    }
    
    return 0;
}
static int set_upg_data(upg_data_t *upg)
{
    int r;
    U32 addr=UPG_INFO_OFFSET;
    
    r = dal_nor_write(addr, upg, sizeof(upg_data_t), 1, 1);
    if(r) {
        LOGE("__set_upg_info, dal_nor_write failed\n");
        return -1;
    }
     
    return 0;
}
static int upg_data_init(upg_data_t *upg)
{
    upg->fwAddr  = APP_OFFSET;
    upg->runAddr = upg->fwAddr+sizeof(upg_data_t);
    upg->facFillFlag = 0;
    upg->head = UPG_HEAD_MAGIC;
    upg->tail = UPG_TAIL_MAGIC;
    upg->upgFlag = UPG_FLAG_NORMAL;
    upg->runFlag = 1;
    upg->upgCnt = 0;
    
    return set_upg_data(upg);
}



static int upg_set_flag(U32 flag)
{
    int r;
    upg_data_t upg;
    
    r = get_upg_data(&upg);
    if(r) {
        LOGE("__upg_set_flag, get_upg_data 1 failed!\n");
        return -1;
    }
    
    upg.upgFlag = flag;
    if(flag==UPG_FLAG_FINISH) {
        upg.upgCnt++;
        upg.runFlag = 1;
    }
    
    r = set_upg_data(&upg);
    if(r) {
        LOGE("__upg_set_flag, get_upg_data 2 failed!\n");
        return -1;
    }
    
    return 0;
}
static int upg_set_fill_flag(U32 flag)
{
    int r;
    upg_data_t upg;
    
    r = get_upg_data(&upg);
    if(r) {
        LOGE("__upg_set_fill_flag, get_upg_data 1 failed!\n");
        return -1;
    }
    
    upg.facFillFlag = flag;
    
    r = set_upg_data(&upg);
    if(r) {
        LOGE("__upg_set_fill_flag, get_upg_data 2 failed!\n");
        return -1;
    }
    LOGD("__upg_set_fill_flag %d ok!\n", flag);
    
    return 0;
}


static int upg_fw_copy(upg_data_t *upg, FW_SRC src)
{
    int r=0;
    U8 *pbuf=MALLOC(FW_LEN);
    U32 tlen=0,rlen,fwlen;
    U32 srcAddr,dstAddr;
    fw_info_t fwInfo;
    
    if(!pbuf) {
        LOGE("__upg_fw_copy, MALLOC failed\n");
        return -1;
    }
    
    srcAddr = (src==FW_SRC_TMP)?TMP_APP_OFFSET:FAC_APP_OFFSET;
    dstAddr = upg->fwAddr;
    
    if(!fw_is_valid(srcAddr, 1)) {
        LOGE("__upg_fw_copy, %s fw is invalid!\n", (src==FW_SRC_TMP)?"tmp":"fac");
        return -1;
    }
    
    r = get_upg_fw_info(srcAddr, &fwInfo);
    if(r) {
        LOGE("__upg_fw_copy, get_upg_fw_info failed\n");
        FREE(pbuf); return -1;
    }
    print_info((src==FW_SRC_TMP)?"tmp":"fac", &fwInfo, NULL);
    
    if(!fw_info_valid(&fwInfo)) {
        LOGE("__upg_fw_copy, fw info is invalid!\n");
        return -1;
    }
    fwlen = fwInfo.length+sizeof(md5_t);
    
    r = dal_nor_erase(dstAddr, fwlen);
    if(r) {
        LOGE("__upg_fw_copy, dal_nor_erase failed, addr: 0x%x, len: %d\n", dstAddr, fwlen);
        return -1;
    }
    
    LOGD("__upg_fw_copy, from 0x%x(sflash) to 0x%x(norflash), len: %d\n", srcAddr, dstAddr, fwlen);
    while(tlen<fwlen) {
        if(tlen+FW_LEN<fwlen) {
            rlen = FW_LEN;
        }
        else {
            rlen = fwlen-tlen;
        }
        
        sflash_read(srcAddr, pbuf, rlen);
        r = dal_nor_write(dstAddr, pbuf, rlen, 0, 1);
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
    FREE(pbuf);
    
    //固件拷贝成功，读取upg区域固件信息
    if(r==0 && tlen>sizeof(fwInfo)) {
        r = get_fw_info(0, &fwInfo);
        if(r==0) {
            print_info("uuu", &fwInfo, upg);
        }
    }
    
    if(r==0) {
        r = upg_set_flag(UPG_FLAG_FINISH);
    }
    
    return r;
}


static int upg_fac_fill(upg_data_t *upg)
{
    int r=0;
    U8 *pbuf=MALLOC(FW_LEN);
    U32 tlen=0,rlen,fwlen;
    U32 srcAddr,dstAddr;
    fw_info_t fwInfo;
    
    if(pbuf) {
        srcAddr = upg->fwAddr;
        dstAddr = FAC_APP_OFFSET;
        
        if(!fw_is_valid(srcAddr, 0)) {
            LOGE("__upg_fac_fill, current fw is invalid!\n");
            return -1;
        }
        
        r = dal_nor_read(srcAddr, &fwInfo, sizeof(fwInfo));
        if(r) {
            LOGE("__upg_fac_fill, read fw info failed\n");
            FREE(pbuf);
            return -1;
        }
        fwlen = fwInfo.length+sizeof(md5_t);
        
        r = sflash_erase(dstAddr, fwlen);
        if(r) {
            LOGD("___ sflash_erase failed, addr: 0x%x, len: %d\n", dstAddr, fwlen);
            return -1;
        }
        
        LOGD("___ upg_fac_fill len %d\n", fwlen);
        while(tlen<fwlen) {
            if(tlen+FW_LEN<fwlen) {
                rlen = FW_LEN;
            }
            else {
                rlen = fwlen-tlen;
            }
            
            dal_nor_read(srcAddr+tlen, pbuf, rlen);
            r = sflash_write(dstAddr+tlen, pbuf, rlen, 0, 1);
            if(r) {
                LOGE("__upg_fill_fac failed addr: 0x%x, len: %d\n", dstAddr, tlen);
                break;
            }
            tlen += rlen;
        }
        FREE(pbuf);
        
        if(r==0) {
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


int upgrade_check(U32 *runAddr, U8 bBoot)
{
    int r=0;
    fw_info_t  fwInfo;
    upg_data_t upgData;
    
    upgrade_init();
    
    r = get_upg_data(&upgData);
    if(r) {
        LOGE("__upgrade_check, get_upg_data failed!\n");
        return -1;
    }
    
    r = get_fw_info(upgData.fwAddr, &fwInfo);
    if(r) {
        LOGE("__upgrade_check, get_fw_info failed!\n");
        return -1;
    }
    
    if(bBoot) {
        print_info("boot", &fwInfo, &upgData);
        
        if(upgData.facFillFlag==0) {
            LOGD("___upg_fac_fill\n");
            
            r = upg_fac_fill(&upgData);
            if(r) {
                LOGE("__upg_fill_fac failed!\n");
            }
            else {
                LOGD("__upg_fill_fac ok!\n");
                
                upgData.facFillFlag = 1;
            }
        }
        
        if(upgData.upgFlag==UPG_FLAG_START) {
            LOGD("__upgrading now !!! copy tmp fw to the run fw area ...\n");
            
            r = upg_fw_copy(&upgData, FW_SRC_TMP);         //copy tmp fw to cur fw
            if(r) {
                LOGE("__upg_fw_copy tmp fw failed!\n");
            }
            else {
                LOGD("__upg_fw_copy tmp fw ok!\n");
                upgData.runFlag = 0;
            }
        }
        
        if(!fw_is_valid(upgData.fwAddr, 0) || (upgData.runFlag && upgData.upgFlag==UPG_FLAG_FINISH)) {
            LOGD("__back to default fw, copy fac fw to the run fw area ...\n");
            
            r = upg_fw_copy(&upgData, FW_SRC_FAC);         //copy fac fw to cur fw
            if(r) {
                LOGE("__upg_fw_copy fac failed!\n");
            }
            else {
                LOGD("__upg_fw_copy fac ok!\n");
            }
        }
    }
    else {
        print_info("app", &fwInfo, &upgData);
        
        if(upgData.upgFlag==UPG_FLAG_FINISH) {
            r = upg_set_flag(UPG_FLAG_NORMAL);
            LOGD("__upg_set_normal_flag: %d\n", r);
        }
    }
    
    if(runAddr) {
        print_info("jump", NULL, &upgData);
        *runAddr = upgData.runAddr;
    }
    LOGD("____ upgrade_check, %d\n", r);
    
    return r;
}


int upgrade_get_fw_info(fw_info_t *info)
{
    if(!info) {
        return -1;
    }
    
    return get_fw_info(0, info);
}


int upgrade_get_upg_data(upg_data_t *upg)
{
    int r;
    
    if(!upg) {
        return -1;
    }
    
    return  get_upg_data(upg);
}


int upgrade_erase(int total_len)
{
    return sflash_erase(TMP_APP_OFFSET, total_len);
}


int upgrade_info_erase(void)
{
    return dal_nor_erase(UPG_INFO_OFFSET, sizeof(upg_data_t));
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
    
    r = sflash_write(addr+upg_tlen, data, len, 1, 1);
    if(r) {
        upg_tlen = 0;
        LOGE("___upgrade_write 0x%x %d failed!\n", addr+upg_tlen, len);
        return -1;
    }
    
    upg_tlen += len;
    LOGD("___upgrade_write tlen: %d\n", upg_tlen);
    
    if(index==0) {
        sflash_read(addr, &fwInfo, sizeof(fwInfo));
        print_info("$$$", &fwInfo, NULL);
    }
    else if(index<0){
        upg_tlen = 0;
        r = upg_set_flag(UPG_FLAG_START);
    }
    
    return r;
}



int upgrade_write_all(U8 *data, int len)
{
    int r;
    U32 addr=TMP_APP_OFFSET;
    
    if(!data || len<=0) {
        return -1;
    }
    
    r = sflash_write(addr, data, len, 0, 1);
    if(r) {
        LOGE("___upgrade_write 0x%x %d failed!\n", addr, len);
        return -1;
    }
    
    r = upg_set_flag(UPG_FLAG_START);
    
    return r;
}



int upgrade_fac_update(void)
{
    return upg_set_fill_flag(0);
}


int upgrade_fw_info_valid(fw_info_t *info)
{
    return fw_info_valid(info);
}


int upgrade_read_fw(buf_t *buf)
{
    int r,fwlen;
    U32 realAddr;
    upg_data_t upg;
    upg_hdr_t  hdr;
    
    if(!buf || !buf->buf || !buf->blen) {
        return -1;
    }
    
    r = get_upg_data(&upg);
    if(r) {
        LOGE("__get_fw_info, get_upg_data failed\n");
        return -1;
    }
    
    r = dal_nor_read(upg.fwAddr, &hdr, sizeof(hdr));
    if(r) {
        LOGE("__get_fw_info, dal_nor_read failed\n");
        return -1;
    }
    
    fwlen = hdr.fw.length+sizeof(md5_t);
    if(buf->blen<fwlen) {
        return -1;
    }
    
    r = dal_nor_read(upg.fwAddr, buf->buf, fwlen);
    if(r==0) {
        buf->dlen = fwlen;
    }
    
    return r;
}

