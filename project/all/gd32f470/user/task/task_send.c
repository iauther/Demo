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

#define RETRY_MAX  3
#define FILE_MAX   500

typedef struct {
    int         cap;
    int         send;
}times_t;


typedef struct {
    int         retry;
    int         fuseup;
    int         busying;
    times_t     times[CH_MAX];
    handle_t    dlist;
    handle_t    flist;
    
    list_node_t *lnode;
}send_handle_t;


send_handle_t sendHandle;
static int file_scan(void);
static int file_upload(void);

static void print_time(char *s, U64 time)
{
    datetime_t dt;
    ts_to_tm(time, &dt);
    rtc2_print_time(s, &dt);
}
static int space_is_full(void)
{
    int r;
    fs_space_t sp;
    
    r = fs_get_space(SDMMC_MNT_PT, &sp);
    if(r) {
        return -1;
    }
    
    LOGD("___ sd space: %lldMB, free: %lldMB\n", sp.total/MB, sp.free/MB);
    if(sp.free>SD_RESERVED_SIZE) {
        return 0;
    }
    
    return 1;
}

static int file_to_flist(char *path)
{
    int r;
    
    r = list_append(sendHandle.flist, 1, path, strlen(path)+1);
    if(r==0) {
        r = list_sort(sendHandle.flist, LIST_SORT_ASCEND);
    }
    
    return r;
}
static int data_save(ch_data_t *pch, int len)
{
    int r;
    char path[100];
    datetime_t dt;
    handle_t fh=NULL;
    int tlen=0,wlen;
    
#ifdef CAP_DATA_SAVE
    ch_para_t *para=paras_get_ch_para(pch->ch);
    
    sendHandle.busying = 1;
    if(space_is_full()){
        sendHandle.busying = 0;
        LOGD("___ space is full!\n");
        return -1;
    }
    
    r = rtc2_get_time(&dt);
    sprintf(path, "%s/%04d/%02d/%02d/ch%d-%llu.bin", SAV_DATA_PATH, dt.date.year, dt.date.mon, dt.date.day, pch->ch, pch->time);
    
    if(fs_exist(path)) {
        LOGW("____ %s already exist, delete it\n", path);
        //fs_remove(path);
        return -1;
    }
    
    fh = fs_open(path, FS_MODE_CREATE);
    if(!fh) {
        sendHandle.busying = 0;
        return -1;
    }
    
    wlen = sizeof(ch_para_t);
    r = fs_write(fh, para, wlen, 0);
    if(r!=wlen) {
        fs_close(fh); sendHandle.busying = 0;
        return -1;
    }
    
    wlen = sizeof(ch_data_t);
    r = fs_write(fh, pch, wlen, 0);
    if(r!=wlen) {
        fs_close(fh); sendHandle.busying = 0;
        return -1;
    }
    
    if(para->upwav) {
        if(pch->wavlen>0) {
            wlen = pch->wavlen;
            r = fs_write(fh, pch->data, wlen, 0);
            if(r!=wlen) {
                fs_close(fh); sendHandle.busying = 0;
                return -1;
            }
        }
    }
    else {
        pch->wavlen = 0;
    }
    
    ts_to_tm(pch->time, &dt);
    rtc2_print_time("___file ts:", &dt);
    
    if(pch->evlen>0) {
        wlen = pch->evlen;
        r = fs_write(fh, (U8*)pch->data+pch->wavlen, wlen, 0);
        if(r!=wlen) {
            fs_close(fh); sendHandle.busying = 0;
            return -1;
        }
    }
    fs_close(fh);
    
    LOGD("___ %s save ok, add it to flist\n", path);
    file_to_flist(path);
    sendHandle.busying = 0;
#endif
    
    return 0;
}
static int data_upload(node_t *node)
{
    handle_t h;
    char *pbuf=NULL;
    int dlen,tlen,r=-1;
    ch_data_t *pch=NULL;
    ch_para_t *para=NULL;
    U32 bfile=node->tp;
    char *path=node->buf;
    mqtt_pub_para_t pub_para={DATA_LT};
    
    if(!api_comm_is_connected()) {
        return -1;
    }
    
    if(bfile) {       //node is file path
        int flen;
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
        r = comm_send_data(tasksHandle.hnet, &pub_para, TYPE_CAP, 0, pch, tlen);
        if(r==0) {
            LOGD("___comm_send_data ok!\n");
            //print_time("__ts__", pch->time);
            
            if(bfile) {
                sendHandle.busying = 2;
                fs_remove(path);
                sendHandle.busying = 0;
            }
            else {
                sendHandle.times[pch->ch].send++;
            }
        }
        else {
            LOGE("___comm_send_data(%s) failed, %d\n", bfile?"file":"buffer", r);
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
    smp_para_t *smp=paras_get_smp();
    
    if(sendHandle.lnode==NULL) {
        //上传数据时，先把文件数据传完再传实时数据
        if(list_size(sendHandle.flist)) {
            xlist = sendHandle.flist;
        }
        else {
            xlist = sendHandle.dlist;
        }
        list_take_node(xlist, &sendHandle.lnode, 0);
    }
    
    if(sendHandle.lnode) {
        node_t *nd=&sendHandle.lnode->data;
        U32 isFile=nd->tp;
        
        r = data_upload(nd);
        if(r==0) {
            sendHandle.retry = 0;
        }
        else {
            if(!isFile) {     //非文件才需存储
                sendHandle.retry++;
                if(sendHandle.retry>=RETRY_MAX) {
                    ch_data_t *pch=(ch_data_t*)nd->buf;
                    data_save(pch, nd->dlen);
                    sendHandle.retry = 0;
                }
            }
            else {
                sendHandle.retry = 1;
            }
        }
        
        if(sendHandle.retry==0) {
            if(nd->blen>100*KB) {       //数据过大，则释放内存
                list_discard_node(xlist, sendHandle.lnode);
            }
            else {
                list_back_node(xlist, sendHandle.lnode);
            }
            sendHandle.lnode = NULL;
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
    list_sort(sendHandle.flist, LIST_SORT_ASCEND);
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
    int r;
    
    if(!sendHandle.dlist) {
        return -1;
    }
    
    r = list_append(sendHandle.dlist, 0, data, len);
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
        
        data_save((ch_data_t*)lnode->data.buf, lnode->data.dlen);
        list_discard_node(sendHandle.dlist, lnode);
    }
    
    return 0;
}



//返回1表示采集结束，返回2表示把本次采集的数据全部发出去
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
    
    file_scan();
    sendHandle.dlist = list_init(&lc);
    sendHandle.lnode = NULL;
    
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
                    data_upload_proc();
                }
                break;
            }
        }
    }
    
}
#endif


