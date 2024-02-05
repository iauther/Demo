#include "list.h"
#include "rtc.h"
#include "fs.h"
#include "mem.h"
#include "comm.h"
#include "paras.h"
#include "cfg.h"
#include "datetime.h"
#include "dal_delay.h"


#ifdef OS_KERNEL

#include "task.h"

//#define USE_FILE
#define FILE_MAX   500

typedef struct {
    int         cap;
    int         send;
}times_t;


typedef struct {
    int         fuseup;
    int         busying;
    times_t     times[CH_MAX];
    handle_t    dlist;
    handle_t    flist;
}send_handle_t;


send_handle_t sendHandle;
static int file_scan(void);
static int file_upload(void);
#define FREE_SIZE   (5*MB)
static int space_is_full(void)
{
    int r;
    fs_space_t sp;
    
    r = fs_get_space(SDMMC_MNT_PT, &sp);
    if(r==0) {
        LOGD("___ sd space: %lldMB, free: %lldMB\n", sp.total/MB, sp.free/MB);
        if(sp.free>=FREE_SIZE) {
            LOGE("___ sd is full, freespace<%dMB\n", FREE_SIZE/MB);
            
            return 0;
        }
    }
    
    return 1;
}

static int data_save_bin(ch_data_t *pch)
{
    int r;
    char path[100];
    datetime_t dt;
    handle_t fh=NULL;
    int flen,tlen;
    ch_para_t *para=paras_get_ch_para(pch->ch);
    
    
    LOGD("___ data save\n");
    sendHandle.busying = 1;
    
#if 0
    if(space_is_full()){
        sendHandle.busying = 0;
        return -1;
    }
#endif
    
    r = rtc2_get_time(&dt);
    sprintf(path, "%s/%04d/%02d/%02d/ch%d-%lld.bin", SAV_DATA_PATH, dt.date.year, dt.date.mon, dt.date.day, pch->ch, pch->time/1000);
    
    if(fs_exist(path)) {
        LOGW("____ %s already exist, delete it\n", path);
        fs_remove(path);
    }
    //LOGD("___ save data to %s\n", path);
    
    fh = fs_open(path, FS_MODE_CREATE);
    if(!fh) {
        //LOGE("___fs_open %s failed\n", path);
        sendHandle.busying = 0;
        return -1;
    }
    
    fs_write(fh, para, sizeof(ch_para_t), 0);
    if(!para->savwav) {
        pch->wavlen = 0;
    }
    fs_write(fh, pch, sizeof(ch_data_t), 0);
    
    if(para->savwav && pch->wavlen>0) {
        fs_write(fh, pch->data, pch->wavlen, 0);
    }
    
    ts_to_tm(pch->time, &dt);
    LOGD("___ pch->time: %lld\n", pch->time);
    rtc2_print_time("data_save ts:", &dt);
    
    if(pch->evlen>0) {
        fs_write(fh, (U8*)pch->data+pch->wavlen, pch->evlen, 0);
    }
    
    fs_close(fh);
    
    flen = fs_length(path);
    LOGD("___ %s length: %d\n", path, flen);
    
    list_append(sendHandle.dlist, 1, path, strlen(path)+1);
    
    sendHandle.busying = 0;
    
    return 0;
}


static int data_save_csv(ch_data_t *pch)
{
    int r,i,j;
    datetime_t dt;
    handle_t fh=NULL;
    char path[100];
    int flen,wlen,maxCnt,maxLen;
    
    if(space_is_full()){
        sendHandle.busying = 0;
        return -1;
    }
    
    r = rtc2_get_time(&dt);

    sprintf(path, "%s/%04d/%02d/%02d/ch%02d.csv", SAV_DATA_PATH, dt.date.year, dt.date.mon, dt.date.day, pch->ch);
    
    flen = fs_length(path);
    if(flen<0) {
        LOGE("___ %s flen: %d\n", path, flen);
        return -1;
    }        
    
    fh = fs_open(path, FS_MODE_CREATE);
    if(!fh) {
        LOGE("___fs_open %s failed\n", path);
        return -1;
    }
    
    //应先写入时间戳和配置(采样率、位宽等)
    //sprintf(tmp, "\"ch:%d, freq: %d, time: %s\"\n", pch->ch, allPara.usr.ch[pch->ch].smpFreq, time);
    //fs_append(xHandle, tmp, strlen(tmp), 0);
    
    int dw=6;
    int offset;

#if 0
    //一个浮点数占位dw个字节
    for(i=0; i<times; i++) {
        for(j=0; j<onelen; j++) {
            offset = i*onelen+j;
            sprintf(fileBuf+j*dw, "%1.03f\n", pch->data[offset]);
        }
        wlen = fs_append(fh, fileBuf, onelen*dw, 0); 
        if(wlen<=0) {
            return -1;
        }
    }
    
    offset = i*onelen;
    for(j=0; j<left; j++) {
        sprintf(fileBuf+j*dw, "%1.03f\n", pch->data[i+j]);
    }
    wlen = fs_append(fh, pch->data, left*dw, 0); 
    if(wlen<=0) {
        return -1;
    }
#endif

    return 0;
}

/////////////////////////////////////////////////////
static int data_upload_test(void)
{
    int r,cnt=10000,tlen;
    mqtt_pub_para_t pub_para={DATA_WAV};
    ch_data_t *pch=(ch_data_t*)eMalloc(cnt*4*2);
    
    if(!pch) {
        return -1;
    }
    
    pch->ch = 0;
    pch->time = rtc2_get_timestamp_ms();
    pch->smpFreq = 1000000;
    pch->wavlen = cnt*4;
    
    tlen = pch->wavlen+sizeof(ch_data_t);
    
    r = comm_send_data(tasksHandle.hcomm, &pub_para, TYPE_CAP, 0, pch, tlen);
    if(r==0) {
        datetime_t dt;
        ts_to_tm(pch->time, &dt);
        
        LOGD("____comm_send_data len: %d, wavlen: %d\n", tlen, pch->wavlen);
        rtc2_print_time("timestamp:", &dt);
        
        
    }
    else {
        LOGE("___ task_send, comm_send_data failed, %d\n", r);
        r = -1;         //发送文件失败
    }
    
    xFree(pch);
    
    return r;
}


static void print_time(char *s, U64 time)
{
    datetime_t dt;
    ts_to_tm(time, &dt);
    rtc2_print_time(s, &dt);
}

static int data_upload(node_t *node)
{
    handle_t h;
    char *pbuf=NULL;
    int dlen,tlen,r=-1;
    ch_data_t *pch=NULL;
    ch_para_t *para=NULL;
    U32 bfile=node->tp;
    mqtt_pub_para_t pub_para={DATA_LT};
    
    if(bfile) {       //node is file path
        int flen;
        char *path=node->buf;
        flen = fs_length(path);
        
        //LOGD("___ data_upload %s, flen: %d\n", path, flen);
        if(flen<=0) {
            r = (flen==0)?0:-1;
            return r;
        }
        
        h = fs_open(path, FS_MODE_RW);
        if(!h) {
            LOGE("___ %s open failed\n", path);
            return -1;                  //打开文件失败
        }
        
        pbuf=eMalloc(flen);
        if(!pbuf) {
            LOGE("___ eMalloc fbuf failed\n");
            return -1;                  //分配内存失败
        }
        fs_read(h, pbuf, flen);
        
        para = (ch_para_t*)pbuf;
        pch  = (ch_data_t*)(pbuf+sizeof(ch_para_t));
    }
    else {
        pch  = (ch_data_t*)node->buf;
        para=paras_get_ch_para(pch->ch);
    }
    
    if(pch->wavlen && !para->upwav) {
        memcpy(pch->data, (U8*)pch->data+pch->wavlen, pch->evlen);
        pch->wavlen = 0;
    }
    dlen = pch->wavlen+pch->evlen;
    
    //LOGD("___wavlen: %d, evlen: %d\n", pch->wavlen, pch->evlen);
    if(dlen>0) {
        
        if(pch->wavlen>0) {
            pub_para.tp = DATA_WAV;
        }
        tlen = dlen+sizeof(ch_data_t);
        
        LOGD("___comm_send_data, len: %d\n", tlen);
        r = comm_send_data(tasksHandle.hcomm, &pub_para, TYPE_CAP, 0, pch, tlen);
        if(r==0) {
            LOGD("___comm_send_data ok!\n");
            //print_time("__ts__", pch->time);
            
            if(!bfile) {
                sendHandle.times[pch->ch].send++;
            }
        }
        else {
            LOGE("___comm_send_data failed, %d\n", r);
            r = -1;         //发送文件失败
        }
    }
    
    if(pbuf) xFree(pbuf);
    
    return r;
}
static int data_upload_proc(void)
{
    int r=0;
    handle_t xlist;
    list_node_t *lnode;
    smp_para_t *smp=paras_get_smp();
    
    if(smp->pwrmode==PWR_PERIOD_PWRDN) {
        if(!api_comm_is_connected()) {
            return -1;
        }
    }
    
#if 0    
    file_scan();
    if(list_size(sendHandle.flist)) {
        xlist = sendHandle.flist;
    }
    else {
        xlist = sendHandle.dlist;
    }
#else
    xlist = sendHandle.dlist;
#endif
    
    r = list_take_node(xlist, &lnode, 0);
    if(r==0) {
        node_t *nd=&lnode->data;
        
        r = data_upload(nd);
        if(r==0) {
            if(nd->tp) {     //如果是文件且发送成功，需删除该文件
                sendHandle.busying = 2;
                r = fs_remove(nd->buf);
                sendHandle.busying = 0;
            }
        }
        else {
            if(smp->pwrmode==PWR_NO_PWRDN) {    //非关机电源模式下需要及时保存数据
                r = data_save_bin((ch_data_t*)nd->buf);
            }
        }
        
        if(r==0) {
            if(nd->blen>50*KB) {       //数据过大，则释放内存
                list_discard_node(sendHandle.dlist, lnode);
            }
            else {
                list_back_node(sendHandle.dlist, lnode);
            }
        }
    }
    
    
    return r;
}
static int iterator_fn(handle_t l, node_t *node, node_t *xd, void *arg, int *act)
{
    int flen;
    *act = ACT_NONE;
    
    //flen = fs_length(xd->buf);
    LOGD("___ %s\n", xd->buf);
    
    return 0;
}
static void file_print(void)
{
    LOGD("___ nvmFlist:\n");
    list_iterator(sendHandle.dlist, NULL, iterator_fn, NULL);
}

//返回1: 文件全部扫完
//返回0: 文件未扫完
static int file_scan(void)
{
    int files=0;

    if(!sendHandle.flist) {
        list_cfg_t lc;
        
        lc.max = FILE_MAX;
        lc.log = 1;
        lc.mode = LIST_FULL_FILO;
        
        sendHandle.flist = list_init(&lc);
    }
    
    if(!sendHandle.flist) {
        return -1;
    }
    
    if(sendHandle.fuseup) {
        return 1;
    }

    fs_scan(SAV_DATA_PATH, sendHandle.flist, &files);
    list_sort(sendHandle.flist, SORT_ASCEND);
    //file_print();
    
    if(files<=FILE_MAX) {
        sendHandle.fuseup = 1;
        return 1;
    }
    
    return 0;
}


static void data_print(void *data, int len)
{
    int i,j;
    char *data_ptr=(char*)data;
    const char* ev_str[EV_NUM] = { "rms","amp","asl","ene","ave","min","max" };
    ch_data_t *pch=(ch_data_t*)data;
    ch_para_t *para=paras_get_ch_para(pch->ch);
    ev_data_t *pev=(ev_data_t*)((U8*)(pch->data)+pch->wavlen);

    if(pch->evlen>0) {
        ev_grp_t *grp=pev->grp;
        for(i=0; i<pev->grps; i++) {
            for(j=0; j<grp->cnt; j++) {
                LOGD("%s[%d]: %0.5f\n", ev_str[grp->val[j].tp], i, grp->val[j].data);
            }
            grp = (ev_grp_t*)((U8*)grp + sizeof(ev_grp_t) + sizeof(ev_val_t)*grp->cnt);
            
            LOGD("\n");
        }
    }
}


//////////////////////////////////////////////////////////////

int api_send_append(void *data, int len)
{
    int r=list_append(sendHandle.dlist, 0, data, len);
    if(r==0) {
        ch_data_t *pch=(ch_data_t*)data;
        sendHandle.times[pch->ch].cap++;
        task_trig(TASK_SEND, EVT_DATA);
    }
    else {
        LOGD("___ api_nvm_send failed\n");
    }
    
    return r;
}

int api_send_save_file(void)
{
    int r;
    list_node_t *lnode=NULL;
    
    while(1) {
        r = list_take_node(sendHandle.dlist, &lnode, 0);
        if(r) {
            break;
        }
        
        r = data_save_bin((ch_data_t*)lnode->data.buf);
        if(r==0) {
            list_discard_node(sendHandle.dlist, lnode);
        }
    }
    
    return 0;
}



//返回1表示采集结束，返回本次采集的数据全部发出去
int api_send_is_finished(void)
{
    U8 ch;
    int finished=0;
    int cap_finished=1;
    int send_finished=1;
    ch_para_t *pch=NULL;
    
    for(ch=0; ch<CH_MAX; ch++) {
        pch = paras_get_ch_para(ch);
        
        if(pch->enable) {
            if(sendHandle.busying || (sendHandle.times[ch].cap<pch->smpTimes)) {
                //LOGD("______ is_finished, ch[%d]: 0, busying: %d, times: %d, smpTimes: %d\n", ch, sendHandle.busying, sendHandle.times[ch].send, pch->smpTimes);
                cap_finished = 0;
                break;
            }
        }
    }
    
    if(list_size(sendHandle.dlist)>0) {
        send_finished = 0;
    }
    
    if(cap_finished) {
        if(send_finished) {
            finished = 2;
        }
        else {
            finished = 1;
        }
    }
    
    return finished;
}


static void send_tmr_callback(void *arg)
{
    task_post(TASK_SEND, NULL, EVT_TIMER, 0, NULL, 0);
}


static void send_init(void)
{
    list_cfg_t lc;
    memset(&sendHandle, 0, sizeof(sendHandle));
    
    lc.max = 20;
    lc.log = 1;
    lc.mode = LIST_FULL_FIFO;
    
    sendHandle.dlist = list_init(&lc);
    
    handle_t htmr = task_timer_init(send_tmr_callback, NULL, 300, -1);
    if(htmr) {
        task_timer_start(htmr);
    }
}

void task_send_fn(void *arg)
{
    int i,r;
    U8  err;
    evt_t e;
    list_node_t *lnode;
    
    LOGD("_____ task send running\n");
    send_init();
    
    while(1) {
        r = task_recv(TASK_SEND, &e, sizeof(e));
        if(r==0) {
            switch(e.evt) {
                
                case EVT_DATA:
                case EVT_TIMER:
                {
                    //读取上传记录文件，上传成功则删除文件
                    data_upload_proc();
                    //data_upload_test();
                }
                break;
                
                case EVT_SAVE:
                {
                    paras_save();
                }
                break;
            }
        }
    }
    
}
#endif


