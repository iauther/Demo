#include "task.h"
#include "dal.h"
#include "log.h"
#include "rtc.h"
#include "net.h"
#include "paras.h"
#include "aiot_ota_api.h"
#include "aiot_dm_api.h"
#include "aiot_ntp_api.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_mqtt_download_api.h"
#include "hal_at_impl.h"
#include "fs.h"
#include "jpatch.h"
#include "upgrade.h"
#include "mqtt.h"
#include "mem.h"
#include "cfg.h"



#define DOWN_USE_HTTP
#define ONCE_PKT_LEN   (4*1024)

#define USE_NTP

//#define MQTT_SSL
#define DOWN_SSL



typedef struct {
    U8  hw;
    U8  init;
    S8  conn;
    U8  ntp;
}flags_t;

typedef struct {
    buf_t buf;
    int   type;
}ota_data_t;


typedef struct {
    handle_t    hmq;
    handle_t    hdm;
    handle_t    hntp;
    handle_t    hota;
    handle_t    hdl;
    
    void        *userdata;
    
    U32         totalen;
    U32         downlen;
}mqtt_handle_t;

#ifdef OS_KERNEL

static flags_t mqttFlag={0};
static U32 ntp_time=0;

extern const char* ali_ca_cert;
static U8 proc_thread_running=0;
static U8 recv_thread_running=0;
static osThreadId_t proc_thread_id=NULL;
static osThreadId_t recv_thread_id=NULL;
static void mqtt_proc_thread(void *args);
static void mqtt_recv_thread(void *args);
static int mqtt_dm_init(mqtt_handle_t *h);
static int mqtt_ntp_init(mqtt_handle_t *h);
static int mqtt_ali_sub(handle_t hconn, void *para);
static int mqtt_ali_req(handle_t hconn);

static int32_t ali_state_logcb(int32_t code, char *message)
{
    LOGD("%s", message);
    return 0;
}

static int set_userdata(mqtt_handle_t *h)
{    
    aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_USERDATA, h->userdata);
    aiot_ntp_setopt(h->hntp, AIOT_NTPOPT_USERDATA, h->userdata);
    aiot_dm_setopt(h->hmq, AIOT_DMOPT_USERDATA, h->userdata);
    
    return 0;
}


/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
static void mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            LOGD("AIOT_MQTTEVT_CONNECT\n");
            mqttFlag.conn = 1;
            
            /* TODO: 处理SDK建连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            LOGD("AIOT_MQTTEVT_RECONNECT\n");
            mqttFlag.conn = 2;
            /* TODO: 处理SDK重连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            LOGD("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
            mqttFlag.conn = 0;
            /* TODO: 处理SDK被动断连, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        default: {

        }
    }
}

////////////////////////////////////////////////////////////////////////////////
static void dm_recv_generic_reply(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    LOGD("demo_dm_recv_generic_reply msg_id = %d, code = %d, data = %.*s, message = %.*s\r\n",
           recv->data.generic_reply.msg_id,
           recv->data.generic_reply.code,
           recv->data.generic_reply.data_len,
           recv->data.generic_reply.data,
           recv->data.generic_reply.message_len,
           recv->data.generic_reply.message);
}

static void dm_recv_property_set(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    LOGD("demo_dm_recv_property_set msg_id = %ld, params = %.*s\r\n",
           (unsigned long)recv->data.property_set.msg_id,
           recv->data.property_set.params_len,
           recv->data.property_set.params);
}

static void dm_recv_async_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    LOGD("dm_recv_async_service_invoke msg_id = %ld, service_id = %s, params = %.*s\r\n",
           (unsigned long)recv->data.async_service_invoke.msg_id,
           recv->data.async_service_invoke.service_id,
           recv->data.async_service_invoke.params_len,
           recv->data.async_service_invoke.params);
}

static void dm_recv_sync_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    LOGD("dm_recv_sync_service_invoke msg_id = %ld, rrpc_id = %s, service_id = %s, params = %.*s\r\n",
           (unsigned long)recv->data.sync_service_invoke.msg_id,
           recv->data.sync_service_invoke.rrpc_id,
           recv->data.sync_service_invoke.service_id,
           recv->data.sync_service_invoke.params_len,
           recv->data.sync_service_invoke.params);
}

static void dm_recv_raw_data(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    LOGD("demo_dm_recv_raw_data raw data len = %d\r\n", recv->data.raw_data.data_len);
}

static void dm_recv_raw_sync_service_invoke(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    LOGD("demo_dm_recv_raw_sync_service_invoke raw sync service rrpc_id = %s, data_len = %d\r\n",
           recv->data.raw_service_invoke.rrpc_id,
           recv->data.raw_service_invoke.data_len);
}

static void dm_recv_raw_data_reply(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    LOGD("demo_dm_recv_raw_data_reply receive reply for up_raw msg, data len = %d\r\n", recv->data.raw_data.data_len);
}


static void dm_recv_handler(void *dm_handle, const aiot_dm_recv_t *recv, void *userdata)
{
    LOGD("demo_dm_recv_handler, type = %d\r\n", recv->type);

    switch (recv->type) {

        case AIOT_DMRECV_GENERIC_REPLY: {
            dm_recv_generic_reply(dm_handle, recv, userdata);
        }
        break;

        case AIOT_DMRECV_PROPERTY_SET: {
            dm_recv_property_set(dm_handle, recv, userdata);
        }
        break;

        case AIOT_DMRECV_ASYNC_SERVICE_INVOKE: {
            dm_recv_async_service_invoke(dm_handle, recv, userdata);
        }
        break;

        case AIOT_DMRECV_SYNC_SERVICE_INVOKE: {
            dm_recv_sync_service_invoke(dm_handle, recv, userdata);
        }
        break;

        case AIOT_DMRECV_RAW_DATA: {
            dm_recv_raw_data(dm_handle, recv, userdata);
        }
        break;

        case AIOT_DMRECV_RAW_SYNC_SERVICE_INVOKE: {
            dm_recv_raw_sync_service_invoke(dm_handle, recv, userdata);
        }
        break;

        case AIOT_DMRECV_RAW_DATA_REPLY: {
            dm_recv_raw_data_reply(dm_handle, recv, userdata);
        }
        break;

        default:
            break;
    }
}
static void ntp_event_handler(void *handle, const aiot_ntp_event_t *event, void *userdata)
{
    switch (event->type) {
        case AIOT_NTPEVT_INVALID_RESPONSE: {
            LOGD("AIOT_NTPEVT_INVALID_RESPONSE\n");
        }
        break;
        case AIOT_NTPEVT_INVALID_TIME_FORMAT: {
            LOGD("AIOT_NTPEVT_INVALID_TIME_FORMAT\n");
        }
        break;
        default: {

        }
    }
}
static void ntp_recv_handler(void *handle, const aiot_ntp_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        /* TODO: 结构体 aiot_ntp_recv_t{} 中包含当前时区下, 年月日时分秒的数值, 可在这里把它们解析储存起来 */
        case AIOT_NTPRECV_LOCAL_TIME: {
            //LOGD("___ AIOT_NTPRECV_LOCAL_TIME\n");
            {
                datetime_t dt;
                
                mqttFlag.ntp = 1;
                LOGD("___ ntp data recved!\n");
                
                dt.date.year = packet->data.local_time.year;
                dt.date.mon = packet->data.local_time.mon;
                dt.date.day = packet->data.local_time.day;
                dt.time.hour = packet->data.local_time.hour;
                dt.time.min = packet->data.local_time.min;
                dt.time.sec = packet->data.local_time.sec;
                dt.time.ms  = 0;
                
                LOGD("ntptime: %4d.%02d.%02d %02d:%02d:%02d\n", 
                                                        packet->data.local_time.year,
                                                        packet->data.local_time.mon,
                                                        packet->data.local_time.day,
                                                        packet->data.local_time.hour,
                                                        packet->data.local_time.min,
                                                        packet->data.local_time.sec);
                
                rtc2_set_time(&dt);
                
                ntp_time = rtc2_get_timestamp_s();
            }       
        }
        break;

        default: {

        }
    }
}

static void set_ota_data(ota_data_t *ota, int type, U32 len)
{
    if(len>0) {
        ota->type = type;
        ota->buf.buf = eMalloc(len);
        ota->buf.blen = len;
        ota->buf.dlen = 0;
    }
    else {
        xFree(ota->buf.buf);
        
        ota->type = -1;
        ota->buf.buf = NULL;
        ota->buf.blen = 0;
        ota->buf.dlen = 0;
    }
}
static void ota_data_cache(ota_data_t *ota, void *data, int len)
{
    if(ota && ota->buf.buf && ota->buf.blen>0) {
        
        //cache data here
        if((ota->buf.dlen+len)<=ota->buf.blen) {
            memcpy(ota->buf.buf+ota->buf.dlen, data, len);
            ota->buf.dlen += len;
        }
    }
}

static void set_buf(buf_t *buf, int len)
{
    if(len>0) {
        buf->blen = len;
        buf->dlen = 0;
        buf->buf  = eMalloc(buf->blen);
    }
    else {
        xFree(buf->buf);
        buf->blen = 0;
        buf->dlen = 0;
        buf->buf = NULL;
    }
}
static void set_jp_data(jp_data_t *jd, buf_t *buf)
{
    jd->buf  = buf->buf;
    jd->size = buf->dlen;
    jd->pos  = 0;
}
static void upgrade_proc(ota_data_t *ota)
{
    int r;
    buf_t sbuf,tbuf;
    int total_upg=0;
    fw_hdr_t *hdr=(fw_hdr_t*)ota->buf.buf;
    
    total_upg = upgrade_fw_info_valid(&hdr->fw);
    
    LOGD("___ total_upg: %d\n", total_upg);
    if(total_upg) {    //完整升级
        r = upgrade_write_all(ota->buf.buf, ota->buf.dlen);
    }
    else {                                      //差分升级
        jp_data_t src,dst,patch;
        
        set_buf(&sbuf, MB); set_buf(&tbuf, MB);
        
        r = upgrade_read_fw(&sbuf);
        if(r) {
            set_buf(&sbuf, 0); set_buf(&tbuf, 0); 
            return;
        }
        
        set_jp_data(&src, &sbuf);               //设置src缓存
        set_jp_data(&patch, &ota->buf);         //设置dst缓存
        
        LOGD("___ src.dlen: %d, patch.dlen: %d\n", src.size, patch.size);
        
        dst.buf = tbuf.buf; dst.size = tbuf.blen; dst.pos = 0;
        r = jpatch(&src, &patch, &dst);
        if(r==0) {
            LOGD("_____ jpatch data len: %d\n", dst.pos);
            //fs_save("/sd/upg/xxx.upg", dst.buf, dst.pos);
            r = upgrade_write_all(dst.buf, dst.pos);
            if(r==0) {
                r = upgrade_read(dst.buf, dst.pos);
                if(r>0) {
                    fs_save("/sd/upg/yyy.upg", dst.buf, r);
                }
            }
        }
    }
    
    if(r==0) {
        LOGD("___ upgrade save file ok, reboot now\n");
        dal_reboot();
    }
}
static void download_recv_handler(void *handle, ota_data_t *ota, int percent, void *data, int len)
{
    int r;
    
    ota_data_cache(ota, data, len);
    
    if (percent == 100) {
        if(ota->type==AIOT_OTARECV_COTA) {
            r = paras_save_json(ota->buf.buf, ota->buf.dlen);
            if(r==0) {
                LOGD("___ cfg file save ok!\n");
            }
        }
        else if(ota->type==AIOT_OTARECV_FOTA) {
            
            LOGD("_________ FOTA FINISHED, data recv len: %d\n", ota->buf.dlen);
            upgrade_proc(ota);
        }
        
        set_ota_data(ota, 0, 0);
    }
}
static int http_download_request(mqtt_handle_t *h, int len)
{
    int r;
    U32 end=h->downlen+len;
    
    aiot_download_setopt(h->hdl, AIOT_DLOPT_RANGE_START, (void *)&h->downlen);
    aiot_download_setopt(h->hdl, AIOT_DLOPT_RANGE_END, (void *)&end);
    
    return aiot_download_send_request(h->hdl);
}
static void http_download_recv_handler(void *handle, const aiot_download_recv_t *packet, void *userdata)
{
    int r;
    ota_data_t *ota=(ota_data_t*)userdata;
    
    /* 目前只支持 packet->type 为 AIOT_DLRECV_HTTPBODY 的情况 */
    if (!packet || AIOT_DLRECV_HTTPBODY != packet->type) {
        return;
    }

    LOGD("http download percent is %d, packet len: %d\n", packet->data.percent, packet->data.len);
    if (packet->data.percent < 0) {
        LOGE("download exception,  %d\n", packet->data.percent);
        return;
    }
    aiot_download_report_progress(handle, packet->data.percent);           //向云端报告进度
    
    download_recv_handler(handle, ota, packet->data.percent, packet->data.buffer, packet->data.len);
}

static void mqtt_download_recv_handler(void *handle, const aiot_mqtt_download_recv_t *packet, void *userdata)
{
    U32 data_buffer_len = 0;
    ota_data_t *ota=(ota_data_t*)userdata;

    /* 目前只支持 packet->type 为 AIOT_DLRECV_HTTPBODY 的情况 */
    if (!packet || AIOT_MDRECV_DATA_RESP != packet->type) {
        return;
    }
    
    LOGD("mqtt download percent is %d, packet len: %d\n", packet->data.data_resp.percent, packet->data.data_resp.data_size);
    if (packet->data.data_resp.percent < 0) {
        LOGE("download exception,  %d\n", packet->data.data_resp.percent);
        return;
    }

    download_recv_handler(handle, ota, packet->data.data_resp.percent, packet->data.data_resp.data, packet->data.data_resp.data_size);
}



static void download_thread(void *arg)
{
    int r=0;
    U32 timeout_ms = 100;
    mqtt_handle_t *h=(mqtt_handle_t*)arg;
    
    aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);
    while (1) {
        
#ifdef DOWN_USE_HTTP
        http_download_request(h, ONCE_PKT_LEN);
        r = aiot_download_recv(h->hdl);
        if(r>0) {
            if(h->downlen+r>h->totalen) {
                r = h->totalen-h->downlen;
            }
            else {
                h->downlen += r;
            }
        }

        /* 全部下载完时, aiot_download_recv() 的返回值会等于 STATE_DOWNLOAD_FINISHED, 否则是当次获取的字节数 */
        if (r==STATE_DOWNLOAD_FINISHED) {
            LOGD("download completed\n");
            break;
        }
#else
        r = aiot_mqtt_download_process(h->hdl);
        if(STATE_MQTT_DOWNLOAD_SUCCESS==r) {
            LOGD("mqtt download ota success \n");
            break;
        } else if(STATE_MQTT_DOWNLOAD_FAILED_RECVERROR == r
                  || STATE_MQTT_DOWNLOAD_FAILED_TIMEOUT == r
                  || STATE_MQTT_DOWNLOAD_FAILED_MISMATCH == r) {
            LOGE("mqtt download ota failed, -0x%x\n", -r);
            break;
        }
#endif
        
    }
    timeout_ms = 5000;
    aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);

    /* 下载所有固件内容完成, 销毁下载会话, 线程自行退出 */
#ifdef DOWN_USE_HTTP
    aiot_download_deinit(&h->hdl);
#else
    aiot_mqtt_download_deinit(&h->hdl);
#endif
    
    h->hdl = NULL;
    
    LOGD("download thread exit\r\n");
    osThreadExit();
}


static void ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    int r;
    mqtt_handle_t *h=(mqtt_handle_t*)userdata;
    int ota_size[2]={1*MB,64*KB};
    static ota_data_t ota_data[2]={{{NULL,0,0},-1},{{NULL,0,0},-1}},*ota;
    
    if (NULL == ota_msg->task_desc) {
        return;
    }
    
    h->totalen = ota_msg->task_desc->size_total;
    h->downlen = 0;
    LOGD("___ ota_recv, type: %s\n", (ota_msg->type==AIOT_OTARECV_FOTA)?"fota":"cota");
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA:     //firmware data
        case AIOT_OTARECV_COTA:     //config data
        {
            if(ota_msg->type==AIOT_OTARECV_FOTA) {
                if(strcmp(ota_msg->task_desc->version, FW_VERSION)==0) {
                    return;
                }
                
                LOGD("___ firmware upgrade, version: %s\n", ota_msg->task_desc->version);
            }
            
#ifdef DOWN_USE_HTTP
            uint16_t port = 443;
            uint32_t max_buf_len = ONCE_PKT_LEN;
            aiot_sysdep_network_cred_t cred;
            
            memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
            cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;
            cred.max_tls_fragment = 16384;
            cred.x509_server_cert = ali_ca_cert;                 /* 用来验证MQTT服务端的RSA根证书 */
            cred.x509_server_cert_len = strlen(ali_ca_cert);     /* 用来验证MQTT服务端的RSA根证书长度 */
            
            h->hdl = aiot_download_init();
            if(h->hdl) {
                aiot_download_setopt(h->hdl, AIOT_DLOPT_NETWORK_CRED, (void *)(&cred));
                aiot_download_setopt(h->hdl, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
                
                r = aiot_download_setopt(h->hdl, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_download_setopt 1 failed\n");
                
                r = aiot_download_setopt(h->hdl, AIOT_DLOPT_RECV_HANDLER, (void *)(http_download_recv_handler));
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_download_setopt 2 failed\n");
                
                r = aiot_download_setopt(h->hdl, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buf_len));
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_download_setopt 3 failed\n");
                
                if(r==STATE_SUCCESS) {
                    ota = &ota_data[ota_msg->type];
                    set_ota_data(ota, ota_msg->type, ota_size[ota_msg->type]);
                }
                
                r = aiot_download_setopt(h->hdl, AIOT_DLOPT_USERDATA, (void *)(ota));
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_download_setopt 4 failed\n");
            
                if(r==STATE_SUCCESS) {
                    LOGD("___ aiot_download_init ok!\n");
                    start_task_simp(download_thread, KB*4, h, NULL);
                }
            }
#else
            h->hdl = aiot_mqtt_download_init();
            if(h->hdl) {
                U32 request_size = 512;
                r = aiot_mqtt_download_setopt(h->hdl, AIOT_MDOPT_TASK_DESC, ota_msg->task_desc);
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_mqtt_download_setopt 1 failed\n");
                
                r = aiot_mqtt_download_setopt(h->hdl, AIOT_MDOPT_DATA_REQUEST_SIZE, &request_size);
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_mqtt_download_setopt 2 failed\n");
                
                r = aiot_mqtt_download_setopt(h->hdl, AIOT_MDOPT_RECV_HANDLE, (void *)(mqtt_download_recv_handler));
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_mqtt_download_setopt 3 failed\n");
                
                if(r==STATE_SUCCESS) {
                    ota = &ota_data[ota_msg->type];
                    set_ota_data(ota, ota_msg->type, ota_size[ota_msg->type]);
                }
                
                r = aiot_mqtt_download_setopt(h->hdl, AIOT_MDOPT_USERDATA, (void *)(ota));
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_mqtt_download_setopt 4 failed\n");
                
                if(r==STATE_SUCCESS) {
                    LOGD("___ aiot_mqtt_download_init ok!\n");
                    start_task_simp(download_thread, KB*4, h, NULL);
                }
            }
#endif
        }
        break;
    }
}


////////////////////////////////////////////////////////////
static int mqtt_dm_init(mqtt_handle_t *h)
{
    uint8_t post_reply = 0;
    
    h->hdm = aiot_dm_init();
    if (h->hdm == NULL) {
        LOGE("aiot_dm_init failed\n");
        return -1;
    }
    aiot_dm_setopt(h->hdm, AIOT_DMOPT_MQTT_HANDLE, h->hmq);
    aiot_dm_setopt(h->hdm, AIOT_DMOPT_RECV_HANDLER, (void *)dm_recv_handler);
    
    //配置是云端否需要回复post_reply给设备. 如果为1, 表示需要云端回复, 否则表示不回复
    aiot_dm_setopt(h->hdm, AIOT_DMOPT_POST_REPLY, (void *)&post_reply);
    
    return 0;
}
static int mqtt_ntp_init(mqtt_handle_t *h)
{
    int res=0;
    int8_t time_zone = 8;
    
#ifdef USE_NTP
    h->hntp = aiot_ntp_init();
    if (h->hntp == NULL) {
        LOGE("aiot_ntp_init failed\n");
        return -1;
    }

    res = aiot_ntp_setopt(h->hntp, AIOT_NTPOPT_MQTT_HANDLE, h->hmq);
    if (res < STATE_SUCCESS) {
        LOGE("aiot_ntp_setopt AIOT_NTPOPT_MQTT_HANDLE failed, res: -0x%04X\n", -res);
        aiot_ntp_deinit(&h->hntp);
        return -1;
    }

    res = aiot_ntp_setopt(h->hntp, AIOT_NTPOPT_TIME_ZONE, (int8_t *)&time_zone);
    if (res < STATE_SUCCESS) {
        LOGE("aiot_ntp_setopt AIOT_NTPOPT_TIME_ZONE failed, res: -0x%04X\n", -res);
        aiot_ntp_deinit(&h->hntp);
        return -1;
    }

    /* TODO: NTP消息回应从云端到达设备时, 会进入此处设置的回调函数 */
    res = aiot_ntp_setopt(h->hntp, AIOT_NTPOPT_RECV_HANDLER, (void *)ntp_recv_handler);
    if (res < STATE_SUCCESS) {
        LOGE("aiot_ntp_setopt AIOT_NTPOPT_RECV_HANDLER failed, res: -0x%04X\n", -res);
        aiot_ntp_deinit(&h->hntp);
        return -1;
    }

    res = aiot_ntp_setopt(h->hntp, AIOT_NTPOPT_EVENT_HANDLER, (void *)ntp_event_handler);
    if (res < STATE_SUCCESS) {
        LOGE("aiot_ntp_setopt AIOT_NTPOPT_EVENT_HANDLER failed, res: -0x%04X\n", -res);
        aiot_ntp_deinit(&h->hntp);
        return -1;
    }
#endif
    
    return 0;
}
static void mqtt_ota_init(mqtt_handle_t *h)
{
    h->hota = aiot_ota_init();
    if (h->hota==NULL) {
        return;
    }

    aiot_ota_setopt(h->hota, AIOT_OTAOPT_MQTT_HANDLE, h->hmq);
    aiot_ota_setopt(h->hota, AIOT_OTAOPT_RECV_HANDLER, ota_recv_handler);
    aiot_ota_setopt(h->hota, AIOT_OTAOPT_USERDATA, h);
    
}

static void start_thread(void *p)
{
    if(!proc_thread_running && !proc_thread_running) {
        LOGD("___ mqtt start proc and recv thread\n");
        proc_thread_running = 1;
        recv_thread_running = 1;
        
        start_task_simp(mqtt_proc_thread, 2*KB, p, &proc_thread_id);
        start_task_simp(mqtt_recv_thread, 2*KB, p, &recv_thread_id);
    }
}
static void stop_thread(void)
{
    proc_thread_running = 0;
    recv_thread_running = 0;
    
    LOGD("___ mqtt stop proc and recv thread\n");
    osDelay(1);
    
    osThreadJoin(proc_thread_id);
    osThreadJoin(recv_thread_id);
}



/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
static void mqtt_proc_thread(void *arg)
{
    int r;
    U32 period,interval;
    mqtt_handle_t *h=(mqtt_handle_t*)arg;

    while (proc_thread_running) {
        r = aiot_mqtt_process(h->hmq);
        if (r == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        
        if(mqttFlag.ntp) {
            period = 3600;
        }
        else {
            period = 2;
        }
        
        interval = rtc2_get_timestamp_s()-ntp_time;
        if(interval>=period) {
            r = aiot_ntp_send_request(h->hntp);
            
            //LOGD("___ ntp_request: %d, ntpflag: %d, interval: %d, period: %d\n", r, mqttFlag.ntp, interval, period);
            ntp_time = rtc2_get_timestamp_s();
        }
        
        osDelay(1);
    }
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
static void mqtt_recv_thread(void *arg)
{
    int32_t res = STATE_SUCCESS;
    mqtt_handle_t *h=(mqtt_handle_t*)arg;

    while (recv_thread_running) {
        res = aiot_mqtt_recv(h->hmq);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            
            if(res==STATE_SYS_DEPEND_NWK_CLOSED) {
                
            }
            
            osDelay(1);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////

static int mqtt_ali_init(void)
{
    int i,r;
    int32_t res = STATE_SUCCESS;
     
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    aiot_state_set_logcb(ali_state_logcb);
    
    r = hal_at_boot();
    
    return r;
}


static int mqtt_ali_deinit(void)
{
    stop_thread();
    
    return 0;
}

static void mqtt_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    conn_handle_t *conn=(conn_handle_t*)userdata;
    
    switch (packet->type) {

        case AIOT_MQTTRECV_PUB: {
            if(conn && conn->para.callback) {
                conn->para.callback(conn, NULL, 0, packet->data.pub.payload, packet->data.pub.payload_len);
            }
        }
        break;
        
        default:
        break;
    }
}


static handle_t mqtt_ali_conn(conn_para_t *para, void *userdata)
{
    int i,r;
    int32_t res = STATE_SUCCESS;
    aiot_sysdep_network_cred_t cred;            /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */
    plat_para_t *plat=&para->para->para.plat;
    mqtt_handle_t *h=NULL;
    
    h = calloc(1, sizeof(mqtt_handle_t));
    if(!h) {
        return NULL;
    }
    h->userdata = userdata;
    
    h->hmq = aiot_mqtt_init();
    if(h->hmq) {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
#ifdef MQTT_SSL
        cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;
#else
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
#endif
        cred.max_tls_fragment = 16384;
        cred.sni_enabled = 1;
        cred.x509_server_cert = ali_ca_cert;                 /* 用来验证MQTT服务端的RSA根证书 */
        cred.x509_server_cert_len = strlen(ali_ca_cert);     /* 用来验证MQTT服务端的RSA根证书长度 */ 
        
        aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_HOST, (void *)plat->host);
        aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_PORT, (void *)&plat->port);
        aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_PRODUCT_KEY, (void *)plat->prdKey);
        aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_DEVICE_NAME, (void *)plat->devKey);
        aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_DEVICE_SECRET, (void *)plat->devSecret);
        aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);

        aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_RECV_HANDLER, (void *)mqtt_recv_handler);
        aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_EVENT_HANDLER, (void *)mqtt_event_handler);
        
        mqtt_dm_init(h);
        mqtt_ntp_init(h);
        mqtt_ota_init(h);
        
        r = aiot_mqtt_connect(h->hmq);
        if(r) {
            aiot_dm_deinit(&h->hdm);
            aiot_ntp_deinit(&h->hntp);
            
            aiot_mqtt_deinit(h->hmq);
            free(h);
            return NULL;
        }
        
        set_userdata(h);
        aiot_ota_report_version(h->hota, FW_VERSION);
        
        mqtt_ali_sub(h, NULL);
        mqtt_ali_req(h);
        
        start_thread(h);
    }
    
    return h;
}


static int mqtt_ali_disconn(handle_t h)
{
    int r;
    mqtt_handle_t *mh=(mqtt_handle_t*)h;
    
    if(!mh) {
        return -1;
    }
    
    aiot_mqtt_disconnect(mh->hmq);
    r = aiot_mqtt_deinit(&mh->hmq);
    if (r < STATE_SUCCESS) {
        LOGE("aiot_mqtt_deinit failed: -0x%04X\n", -r);
        return -1;
    }
    
    aiot_dm_deinit(&mh->hdm);
    
#ifdef USE_NTP
    aiot_ntp_deinit(&mh->hntp);
#endif
    
    stop_thread();
    
    
    return 0;
}


static int mqtt_ali_sub(handle_t hconn, void *para)
{
    int r;
    char tmp[100];
    mqtt_handle_t *h=(mqtt_handle_t*)hconn;
    conn_handle_t *conn=(conn_handle_t*)h->userdata;
    plat_para_t *plat=&conn->para.para->para.plat;
    
    if(!h) {
        return -1;
    }

    sprintf(tmp, "/%s/%s/user/get", plat->prdKey, plat->devKey);
    r = aiot_mqtt_sub(h->hmq, tmp, NULL, 1, h->userdata);
    
    return (r<0)?-1:0;
}


static int mqtt_ali_pub(handle_t hconn, void *para, void *data, int len)
{
    char tmp[100];
    int r,tlen=0;
    aiot_dm_msg_t msg;
    mqtt_handle_t *h=(mqtt_handle_t*)hconn;
    conn_handle_t *conn=(conn_handle_t*)h->userdata;
    plat_para_t *plat=&conn->para.para->para.plat;
    mqtt_pub_para_t *pp=(mqtt_pub_para_t*)para;

    if(!h || !para || !data || !len) {
        return -1;
    }
    
    if(!mqttFlag.conn) {
        return -1;
    }

    if(pp->tp==DATA_LT) {
        memset(&msg, 0, sizeof(aiot_dm_msg_t));
        msg.type = AIOT_DMMSG_RAW_DATA;
        msg.data.raw_data.data = (U8*)data;
        msg.data.raw_data.data_len = len;

        r = aiot_dm_send(h->hdm, &msg);
    }
    else {
        if(pp->tp==DATA_WAV) {
            sprintf(tmp, "/%s/%s/user/wav", plat->prdKey, plat->devKey);
        }
        else {
            sprintf(tmp, "/%s/%s/user/set", plat->prdKey, plat->devKey);
        }
        r = aiot_mqtt_pub(h->hmq, tmp, data, len, 1);
    }
    
    return (r<0)?-1:0;
}


static int mqtt_ali_req(handle_t hconn)
{
    int r=0;
    char tmp[100];
    char payload[100];
    char *request1 = "{\"id\":%d,\"params\":{\"configScope\":\"product\",\"getType\":\"file\"},\"method\":\"thing.config.get\"}";
    char *request2 = "{\"id\":%d,\"version\":\"1.0\",\"params\":{\"module\":\"MCU\"},\"method\":\"thing.ota.firmware.get\"}";
    mqtt_handle_t *h=(mqtt_handle_t*)hconn;
    conn_handle_t *conn=(conn_handle_t*)h->userdata;
    plat_para_t *plat=&conn->para.para->para.plat;
    
    sprintf(tmp, "/sys/%s/%s/thing/config/get", plat->prdKey, plat->devKey);
    sprintf(payload, request1, allPara.sys.devInfo.devID);
    //r = aiot_mqtt_pub(h->hmq, tmp, (U8*)payload, strlen(payload), 0);
    
    sprintf(tmp, "/sys/%s/%s/thing/ota/firmware/get", plat->prdKey, plat->devKey);
    sprintf(payload, request2, allPara.sys.devInfo.devID);
    r = aiot_mqtt_pub(h->hmq, tmp, (U8*)payload, strlen(payload), 0);
    
    return r;
}


static int mqtt_ali_ntp_synced(void)
{
    return mqttFlag.ntp;
}


mqtt_fn_t mqtt_ali_fn={
    mqtt_ali_init,
    mqtt_ali_deinit,
    
    mqtt_ali_conn,
    mqtt_ali_disconn,
    mqtt_ali_sub,
    mqtt_ali_pub,
    mqtt_ali_req,
    mqtt_ali_ntp_synced,
};



#endif


