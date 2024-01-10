#include "upgrade.h"
#include "sflash.h"
#include "dal_nor.h"
#include "sflash.h"
#include "md5.h"
#include "dal.h"
#include "log.h"
#include "cfg.h"


#define FW_LEN                  (32*KB)

#define MALLOC   malloc
#define FREE     free



typedef int (*fw_erase_t)(U32 addr, U32 len);
typedef int (*fw_read_t)(U32 addr, void *data, U32 len);
typedef int (*fw_write_t)(U32 addr, void *data, U32 len, U8 erase, U8 chk);
typedef struct {
    fw_read_t   read;
    fw_write_t  write;
    fw_erase_t  erase;
    
    char        *txt;
    U32         haddr;
    U32         baddr;
}fw_handle_t;


static int upg_init_flag=0;
static int get_all(upg_all_t *all);
static fw_handle_t fwHandle[FW_MAX]={
    {
        dal_nor_read,
        dal_nor_write,
        dal_nor_erase,
        
        "cur",
        UPG_INFO_OFFSET+offsetof(upg_all_t,hdr),
        APP_OFFSET,
    },
    
    {
        sflash_read,
        sflash_write,
        sflash_erase,
        
        "fac",
        FAC_APP_OFFSET,
        FAC_APP_OFFSET+sizeof(fw_hdr_t),
    },

    {
        sflash_read,
        sflash_write,
        sflash_erase,
        
        "tmp",
        TMP_APP_OFFSET,
        TMP_APP_OFFSET+sizeof(fw_hdr_t),
    }
};
static fw_handle_t* get_handle(int fw)
{
    if(fw<FW_CUR || fw>=FW_MAX) {
        return NULL;
    }
    
    return fwHandle+fw;
}

static void upg_delay(U8 flag)
{
    if(flag) {
        int dly=10000;
        while(dly--);
    }
}
static void print_info(char *s, fw_info_t *fw, upg_data_t *upg)
{
    if(fw) {
        LOGD("\n");
        LOGD("__%s__fwInfo->magic:   0x%08x\n",    s, fw->magic);
        LOGD("__%s__fwInfo->version: %s\n",        s, fw->version);
        LOGD("__%s__fwInfo->bldtime: %s\n",        s, fw->bldtime);
        LOGD("__%s__fwInfo->length:  %d\n",        s, fw->length);
        LOGD("\n");
    }
    
    if(upg) {
        if(!fw) {
            LOGD("\n");
        }
        
        LOGD("__%s__upgInfo->head: 0x%08x\n",      s, upg->head);
        LOGD("__%s__upgInfo->tail: 0x%08x\n",      s, upg->tail);
        LOGD("__%s__upgInfo->fwAddr: 0x%08x\n",    s, upg->fwAddr);
        LOGD("__%s__upgInfo->upgCnt: %d\n",        s, upg->upgCnt);
        LOGD("__%s__upgInfo->upgFlag: %d\n",       s, upg->upgFlag);
        LOGD("__%s__upgInfo->facFillFlag: %d\n",   s, upg->facFillFlag);
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


static int all_valid(upg_all_t *all)
{
    int r;
    mcu_info_t mInfo;

    r = dal_get_info(&mInfo);
    if(r) {
        LOGE("__upg_data_valid, dal_get_info failed!\n");
        return 0;
    }
    
    if(all->upg.head!=UPG_HEAD_MAGIC) {
        LOGE("__upg_data_valid, upg data.head: 0x%08x is wrong, correct is: 0x%08x\n", all->upg.head, UPG_HEAD_MAGIC);
        return 0;
    }
    
    if(all->upg.tail!=UPG_TAIL_MAGIC) {
        LOGE("__upg_data_valid, upg data.tail: 0x%08x is wrong, correct is: 0x%08x\n", all->upg.head, UPG_TAIL_MAGIC);
        return 0;
    }

    if(all->upg.upgFlag>=UPG_FLAG_MAX) {
        LOGE("__upg_data_valid, upg data.upgFlag: %d is wrong, correct is: 0~%d\n", all->upg.upgFlag, UPG_FLAG_MAX);
        return 0;
    }
    
    if(all->upg.facFillFlag>1) {
        LOGE("__upg_data_valid, upg data.facFillFlag: %d is wrong, correct is: 0 or 1\n", all->upg.facFillFlag);
        return 0;
    }

    if((all->upg.fwAddr<mInfo.flashStart) || (all->upg.fwAddr>mInfo.flashEnd)) {
        LOGE("__upg_data_valid, upg data.fwAddr: 0x%08x is wrong, correct is: 0x%08x~0x%08x\n", all->upg.fwAddr, mInfo.flashStart, mInfo.flashEnd);
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
static int fw_check(fw_hdr_t *hdr)
{
    U8 i,tmp[16];
    md5_t m1,m2;
    md5_ctx_t ctx;

    if(!fw_info_valid(&hdr->fw)) {
        LOGE("__fw_check, fw info is invalid!\n");
        return -1;
    }
    
    md5_init(&ctx);
    md5_update(&ctx, hdr->data, hdr->fw.length);
    
    memcpy(&m1, hdr->data+hdr->fw.length, sizeof(m1));

    md5_final(&ctx, tmp);
    for (i=0; i<16; i++) {
        byte2char(&m2.digit[i*2], tmp[i]);
    }
    
    print_md5("ori-md5: ", &m1);
    print_md5("cal-md5: ", &m2);
    
    if(memcmp(&m1, &m2, sizeof(md5_t))) {
        return -1;
    }
    
    return 0;
}


static int fw_is_valid(int fw, fw_hdr_t *fh)
{
    U8 i,tmp[16];
    int r,rlen,tlen=0;
    md5_t m1,m2;
    U8 *pbuf=NULL;
    fw_hdr_t hdr;
    md5_ctx_t ctx;
    fw_handle_t *h=get_handle(fw);
    
    r = h->read(h->haddr, &hdr, sizeof(fw_hdr_t));
    if(r) {
        LOGE("___ fw_is_valid, read failed 0\n");
        return 0;
    }
    print_info(h->txt, &hdr.fw, NULL);
    
    if(!fw_info_valid(&hdr.fw)) {
        LOGE("___ fw_is_valid, fw info is invalid!\n");
        return 0;
    }
    
    pbuf = (U8*)MALLOC(FW_LEN);
    if(!pbuf) {
        LOGE("___ fw_is_valid, MALLOC falied\n");
        return 0;
    }
    
    md5_init(&ctx);
    md5_update(&ctx, (U8*)&hdr, sizeof(fw_hdr_t));
    
    while(1) {
        
        if(tlen>=hdr.fw.length) {
            break;
        }
        
        if(tlen+FW_LEN<hdr.fw.length) {
            rlen = FW_LEN;
        }
        else {
            rlen = hdr.fw.length-tlen;
        }
        
        r = h->read(h->baddr+tlen, pbuf, rlen);
        if(r) {
            LOGE("___ fw_is_valid, read failed 1\n");
            break;
        }
        tlen += rlen;
        
        md5_update(&ctx, pbuf, rlen);
    }
    FREE(pbuf);
    
    if(r==0) {
        h->read(h->baddr+hdr.fw.length, &m1, sizeof(m1));
    
        md5_final(&ctx, tmp);
        for (i=0; i<16; i++) {
            byte2char(&m2.digit[i*2], tmp[i]);
        }
        
        print_md5("ori-md5: ", &m1);
        print_md5("cal-md5: ", &m2);
        
        if(memcmp(&m1, &m2, sizeof(md5_t))) {
            LOGE("___fw_is_valid md5 is wrong!\n");
            r = -1;
        }
    }
    
    if(r==0 && fh) {
        *fh = hdr;
    }
    
    return (r==0)?1:0;
}

/*
    fwAddr=0, 从upg升级信息区读固件信息
    fwAddr>0, 视该地址为片内flash地址，从该地址读取固件信息
*/
static int get_fw_info(int fw, fw_hdr_t *hdr)
{
    fw_handle_t *h=get_handle(fw);
    return h->read(h->haddr, hdr, sizeof(fw_hdr_t));
}

static int get_all(upg_all_t *all)
{
    int r;
    
    r = dal_nor_read(UPG_INFO_OFFSET, (U8*)all, sizeof(upg_all_t));
    if(r) {
        LOGE("___get_all, dal_nor_read failed\n");
        return -1;
    }
    
    if(!all_valid(all)) {
        LOGE("___get_all, upg info is invalid\n");
        return -1;
    }
    
    return 0;
}
static int set_all(upg_all_t *all)
{
    int r;
    U32 addr=UPG_INFO_OFFSET;
    
    r = dal_nor_write(addr, all, sizeof(upg_all_t), 1, 1);
    if(r) {
        LOGE("___set_all, dal_nor_write failed\n");
        return -1;
    }
     
    return 0;
}
static int upg_data_init(upg_all_t *all)
{
    all->upg.fwAddr  = APP_OFFSET;
    all->upg.facFillFlag = 0;
    all->upg.head = UPG_HEAD_MAGIC;
    all->upg.tail = UPG_TAIL_MAGIC;
    all->upg.upgFlag = UPG_FLAG_NORMAL;
    all->upg.runFlag = 1;
    all->upg.upgCnt = 0;
    
    return set_all(all);
}



static int upg_set_flag(U32 flag)
{
    int r;
    upg_all_t all;
    
    r = get_all(&all);
    if(r) {
        LOGE("__upg_set_flag, get_upg_data 1 failed!\n");
        return -1;
    }
    
    all.upg.upgFlag = flag;
    if(flag==UPG_FLAG_FINISH) {
        all.upg.upgCnt++;
        all.upg.runFlag = 1;
    }
    
    r = set_all(&all);
    if(r) {
        LOGE("__upg_set_flag, get_upg_data 2 failed!\n");
        return -1;
    }
    
    return 0;
}
static int upg_set_fill_flag(U32 flag)
{
    int r;
    upg_all_t all;
    
    r = get_all(&all);
    if(r) {
        LOGE("__upg_set_fill_flag, get_upg_data 1 failed!\n");
        return -1;
    }
    
    all.upg.facFillFlag = flag;
    
    r = set_all(&all);
    if(r) {
        LOGE("__upg_set_fill_flag, get_upg_data 2 failed!\n");
        return -1;
    }
    LOGD("__upg_set_fill_flag %d ok!\n", flag);
    
    return 0;
}


static int upg_fw_copy(upg_all_t *all, int fwSrc, int fwDst)
{
    int r=0;
    fw_hdr_t hdr,hdr2;
    U32 rlen,fwlen,tlen=0;
    fw_handle_t *hsrc,*hdst;
    U8 *pbuf=MALLOC(FW_LEN);
    
    if(!pbuf || fwSrc==fwDst) {
        LOGE("___upg_fw_copy, para is wrong!\n");
        return -1;
    }
    hsrc = get_handle(fwSrc); hdst = get_handle(fwDst);
    LOGD("___upg_fw_copy, from %s to %s\n", hsrc->txt, hdst->txt);
    
    if(!fw_is_valid(fwSrc, &hdr)) {
        LOGE("___upg_fw_copy, %s fw is invalid!\n", hsrc->txt);
        return -1;
    }
    
    LOGD("___upg_fw_copy, erase hdr area ...\n");
    r = hdst->erase(hdst->haddr, sizeof(hdr));
    if(r) {
        LOGE("___upg_fw_copy, erase hdr area failed, addr: 0x%x, len: %d\n", hdst->haddr, sizeof(hdr));
        return -1;
    }
    
    LOGD("___upg_fw_copy, erase fw area ...\n");
    fwlen = hdr.fw.length+sizeof(md5_t);
    r = hdst->erase(hdst->baddr, fwlen);
    if(r) {
        LOGE("___upg_fw_copy, erase fw area failed, addr: 0x%x, len: %d\n", hdst->baddr, fwlen);
        return -1;
    }
    
    //先写入fw_hdr_t结构的固件头
    LOGD("___upg_fw_copy, copy fw hdr ...\n");
    r = hdst->write(hdst->haddr, &hdr, sizeof(hdr), 0, 1);
    if(r) {
        LOGE("___upg_fw_copy, write hdr failed, addr: 0x%x, len: %d\n", hdst->haddr, fwlen);
        return -1;
    }
    
    //再写入固件+md5码
    LOGD("___upg_fw_copy, copy fw data ...\n");
    while(tlen<fwlen) {
        if(tlen+FW_LEN<fwlen) {
            rlen = FW_LEN;
        }
        else {
            rlen = fwlen-tlen;
        }
        
        hsrc->read(hsrc->baddr+tlen, pbuf, rlen);
        r = hdst->write(hdst->baddr+tlen, pbuf, rlen, 0, 1);
        if(r==0) {
            tlen += rlen;
            //LOGD("__upg_fw_copy total len: %d\n", tlen);
        }
        else {
            LOGE("__upg_fw_copy failed, from 0x%x to 0x%x len: %d\n", hsrc->baddr+tlen, hdst->baddr+tlen, rlen);
            break;
        }
    }
    LOGD("___upg_fw_copy, copy finished\n");
    FREE(pbuf);
    
    //固件拷贝成功，读取upg区域固件信息
    if(r==0) {
        r = get_fw_info(FW_CUR, &hdr);
        if(r==0) {
            all->hdr = hdr;
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


int upgrade_check(U32 *fwAddr, U8 bBoot)
{
    int r=0;
    upg_all_t  all;
    
    upgrade_init();
    
    r = get_all(&all);
    if(r) {
        LOGE("__upgrade_check, get_all failed!\n");
        return -1;
    }
    
    if(bBoot) {
        print_info("boot", &all.hdr.fw, &all.upg);
        
        if(all.upg.facFillFlag==0) {
            LOGD("___upg_fac_fill\n");
            
            r = upg_fw_copy(&all, FW_CUR, FW_FAC);
            if(r) {
                LOGE("__upg_fill_fac failed!\n");
            }
            else {
                LOGD("__upg_fill_fac ok!\n");
                
                upg_set_fill_flag(1);
            }
        }
        
        if(all.upg.upgFlag==UPG_FLAG_START) {
            LOGD("__upgrading now !!! copy tmp fw to the run fw area ...\n");
            
            r = upg_fw_copy(&all, FW_TMP, FW_CUR);         //copy tmp fw to cur fw
            if(r) {
                LOGE("__upg_fw_copy tmp fw failed!\n");
            }
            else {
                LOGD("__upg_fw_copy tmp fw ok!\n");
                
                upg_set_flag(UPG_FLAG_FINISH);
                all.upg.runFlag = 0;
            }
        }
        
        if(!fw_is_valid(FW_CUR, NULL) || (all.upg.runFlag && all.upg.upgFlag==UPG_FLAG_FINISH)) {
            LOGD("__back to default fw, copy fac fw to the run fw area ...\n");
            
            r = upg_fw_copy(&all, FW_FAC, FW_CUR);         //copy fac fw to cur fw
            if(r) {
                LOGE("__upg_fw_copy fac failed!\n");
            }
            else {
                LOGD("__upg_fw_copy fac ok!\n");
            }
        }
    }
    else {
        print_info("app", &all.hdr.fw, &all.upg);
        
        if(all.upg.upgFlag==UPG_FLAG_FINISH) {
            r = upg_set_flag(UPG_FLAG_NORMAL);
            LOGD("__upg_set_normal_flag: %d\n", r);
        }
    }
    
    if(fwAddr) {
        print_info("jump", NULL, &all.upg);
        upg_delay(bBoot);
        
        *fwAddr = all.upg.fwAddr;
    }
    
    return r;
}


int upgrade_get_fw_info(int fw, fw_hdr_t *hdr)
{
    if(!hdr) {
        return -1;
    }
    
    return get_fw_info(fw, hdr);
}


int upgrade_get_all(upg_all_t *all)
{
    int r;
    
    if(!all) {
        return -1;
    }
    
    return  get_all(all);
}


int upgrade_erase(int total_len)
{
    return sflash_erase(TMP_APP_OFFSET, total_len);
}


int upgrade_info_erase(void)
{
    return dal_nor_erase(UPG_INFO_OFFSET, sizeof(upg_data_t));
}


int upgrade_read(U8 *data, int len)
{
    int r;
    U32 addr=TMP_APP_OFFSET;
    
    r = sflash_read(addr, data, len);
    if(r) {
        return -1;
    }
    
    return len;
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
    
    if(index<0){
        upg_tlen = 0;
        r = upg_set_flag(UPG_FLAG_START);
    }
    
    return r;
}



int upgrade_write_all(U8 *data, int len)
{
    int r;
    U32 addr=TMP_APP_OFFSET;
    fw_hdr_t *hdr=(fw_hdr_t*)data;
    
    if(!data || len<=0) {
        return -1;
    }
    
    if(fw_check(hdr)) {
        return -1;
    }
    
    r = upgrade_erase(len);
    if(r) {
        LOGE("___upgrade_write_all, upgrade_erase 0x%x %d failed!\n", addr, len);
        return -1;
    }
    
    r = sflash_write(addr, data, len, 0, 1);
    if(r) {
        LOGE("___upgrade_write_all, sflash_write  0x%x %d failed!\n", addr, len);
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
    upg_all_t all;
    
    if(!buf || !buf->buf || !buf->blen) {
        return -1;
    }
    
    r = get_all(&all);
    if(r) {
        LOGE("___upgrade_read_fw, get_upg_data failed\n");
        return -1;
    }
    
    fwlen = all.hdr.fw.length+sizeof(md5_t);
    if(buf->blen<fwlen) {
        return -1;
    }
    
    r = dal_nor_read(all.upg.fwAddr, buf->buf, fwlen);
    if(r==0) {
        buf->dlen = fwlen;
    }
    
    return r;
}


void upgrade_test(void)
{
    U32 i,tlen=0;
    #define TCNT        50000
    #define TLEN        (TCNT*4)
    U32 addr;
    U32 *pbuf=(U32*)malloc(TLEN);
    
    if(!pbuf) {
        return;
    }
    
    upgrade_init();
    for(i=0; i<TCNT; i++) {
        pbuf[i] = i;
    }
    
    upgrade_write_all((U8*)pbuf, TLEN);
    
    free(pbuf);
}


