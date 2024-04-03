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
#include "dal_delay.h"
#include "fs.h"
#include "jpatch.h"
#include "upgrade.h"
#include "mqtt.h"
#include "mem.h"
#include "cfg.h"



#define DOWN_USE_HTTP
#define DOWN_USE_MQTT

#define ONCE_PKT_LEN   (20*1024)

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
    cfg_file_t cfile;
    
    handle_t h;     //mqtt_handle_t
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
    
    flags_t     flags;
    U32         ntime;
    
    U32         reqflag;
    U8          mode;
    int         ota_proto;
}mqtt_handle_t;

#ifdef OS_KERNEL

extern const char* ali_ca_cert;
static U8 proc_thread_running=0;
static U8 recv_thread_running=0;
static osThreadId_t proc_thread_id=NULL;
static osThreadId_t recv_thread_id=NULL;
static void mqtt_proc_thread(void *args);
static void mqtt_recv_thread(void *args);
static int mqtt_dm_init(mqtt_handle_t *h);
static int mqtt_ntp_init(mqtt_handle_t *h);
static int mqtt_subx(handle_t hconn, void *para);
static int mqtt_req(handle_t hconn, int req);
static int mqtt_reconn(handle_t hconn);

static int32_t ali_state_logcb(int32_t code, char *message)
{
    LOGD("%s", message);
    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
static void mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    mqtt_handle_t *h=(mqtt_handle_t*)userdata;
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            LOGD("AIOT_MQTTEVT_CONNECT\n");
            h->flags.conn = 1;
            
            /* TODO: 处理SDK建连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            LOGD("AIOT_MQTTEVT_RECONNECT\n");
            h->flags.conn = 2;
            /* TODO: 处理SDK重连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            LOGD("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
            h->flags.conn = -1;
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
    mqtt_handle_t *h=(mqtt_handle_t*)userdata;
        
    switch (packet->type) {
        /* TODO: 结构体 aiot_ntp_recv_t{} 中包含当前时区下, 年月日时分秒的数值, 可在这里把它们解析储存起来 */
        case AIOT_NTPRECV_LOCAL_TIME: {
            //LOGD("___ AIOT_NTPRECV_LOCAL_TIME\n");
            {
                datetime_t dt;
                
                h->flags.ntp = 1;
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
                h->ntime = rtc2_get_timestamp_s();
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
static int upg_proc(ota_data_t *ota)
{
    int r=0;
    buf_t sbuf,tbuf,xbuf;
    int total_upg=0;
    fw_hdr_t *hdr=(fw_hdr_t*)ota->buf.buf;
    
    total_upg = upgrade_fw_info_valid(&hdr->fw);
    
    LOGD("___ total_upg: %d\n", total_upg);
    if(total_upg) {    //完整升级
        xbuf.buf = ota->buf.buf;
        xbuf.dlen = ota->buf.dlen;
    }
    else {                                      //差分升级
        jp_data_t src,dst,patch;
        
        set_buf(&sbuf, MB); set_buf(&tbuf, MB);
        
        r = upgrade_read_fw(&sbuf);
        if(r) {
            set_buf(&sbuf, 0); set_buf(&tbuf, 0); 
            return -1;
        }
        
        set_jp_data(&src, &sbuf);               //设置src缓存
        set_jp_data(&patch, &ota->buf);         //设置dst缓存
        
        LOGD("___ src.dlen: %d, patch.dlen: %d\n", src.size, patch.size);
        dst.buf = tbuf.buf; dst.size = tbuf.blen; dst.pos = 0;
        r = jpatch(&src, &patch, &dst);
        if(r==0) {
            LOGD("_____ jpatch data len: %d\n", dst.pos);
            xbuf.buf = dst.buf;
            xbuf.dlen = dst.pos;
        }
    }
    
    if(r==0) {
        r = upgrade_write_all(xbuf.buf, xbuf.dlen);
        if(r) {
            LOGE("___ upgrade_write_all failed\n");
            //fs_save("/sd/new.upg", xbuf.buf, xbuf.dlen);
            //fs_save("/sd/old.upg", sbuf.buf, sbuf.dlen);
        }
    }
    
    if(total_upg==0) {
        set_buf(&sbuf, 0); set_buf(&tbuf, 0); 
    }
    
    
    return r;
}
static void download_recv_handler(void *handle, ota_data_t *ota, int percent, void *data, int len)
{
    int r;
    mqtt_handle_t *h=(mqtt_handle_t*)ota->h;
    
    ota_data_cache(ota, data, len);
    
    if (percent == 100) {
        if(ota->type==AIOT_OTARECV_COTA) {
            cfg_file_t *cf = paras_get_cfile();
            smp_para_t* smp = paras_get_smp();
            
            *cf = ota->cfile;
            r = paras_save_json(ota->buf.buf, ota->buf.dlen);
            if(r>=0) {
                h->reqflag |= MQTT_REQ_CFG;
                LOGD("___ cfg file save ok!\n");
                
                if(r>0 || smp->pwrmode==PWR_NO_PWRDN) {
                    dal_reboot();
                }
            }
            
            //paras_set_rtmin(0);
        }
        else if(ota->type==AIOT_OTARECV_FOTA) {
            LOGD("_________ FOTA FINISHED, data recv len: %d\n", ota->buf.dlen);
            
            r = upg_proc(ota);
            if(r==0) {
                h->reqflag |= MQTT_REQ_FW;
                LOGD("___ upgrade write ok, reboot now\n");
                
                dal_reboot();
            }
            paras_set_rtmin(0);
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

static int mqtt_download_request(mqtt_handle_t *h, int len)
{
    int r;
    U32 end=h->downlen+len;
    
    aiot_mqtt_download_setopt(h->hdl, AIOT_MDOPT_RANGE_START, (void *)&h->downlen);
    aiot_mqtt_download_setopt(h->hdl, AIOT_MDOPT_RANGE_END, (void *)&end);
    
    return 0;
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


static void ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    int r;
    handle_t hdl=NULL;
    mqtt_handle_t *h=(mqtt_handle_t*)userdata;
    conn_handle_t *conn=(conn_handle_t*)h->userdata;
    int ota_size[2]={1*MB,64*KB};
    static ota_data_t ota_data[2]={{{NULL,0,0},-1,NULL},{{NULL,0,0},-1,NULL}},*ota;
    
    if (NULL == ota_msg->task_desc) {
        return;
    }
    
    h->ota_proto = -1;
    h->totalen = ota_msg->task_desc->size_total;
    h->downlen = 0;
    LOGD("___ ota_recv, type: %s\n", (ota_msg->type==AIOT_OTARECV_FOTA)?"fota":"cota");
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA:     //firmware data
        case AIOT_OTARECV_COTA:     //config data
        {
            ota = &ota_data[ota_msg->type];
            set_ota_data(ota, ota_msg->type, ota_size[ota_msg->type]);
            ota->h = h;
            
            if(ota_msg->type==AIOT_OTARECV_FOTA) {
                
                if(strcmp(ota_msg->task_desc->version, FW_VERSION)==0) {
                    return;
                }
                
                paras_set_rtmin(300);
                LOGD("___ firmware upgrade, version: %s\n", ota_msg->task_desc->version);
            }
            else {
                cfg_file_t *cf = paras_get_cfile();
                
                if((ota_msg->task_desc->version==NULL || strlen(ota_msg->task_desc->version)==0) || strcmp((char*)cf->cid, ota_msg->task_desc->version)==0) {
                    LOGD("___ config file no need to update\n");
                    paras_set_rtmin(0);
                    return;
                }
                
                paras_set_rtmin(60);
                strcpy((char*)ota->cfile.cid, ota_msg->task_desc->version);
            }
            
            if(ota_msg->task_desc->protocol_type == AIOT_OTA_PROTOCOL_HTTPS) {
                hdl = aiot_download_init();
                if(!hdl) {
                    return;
                }
                
                uint16_t port = 443;
                uint32_t max_buf_len = ONCE_PKT_LEN;
                aiot_sysdep_network_cred_t cred;
                
                memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
                cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;
                cred.max_tls_fragment = 16384;
                cred.x509_server_cert = ali_ca_cert;                 /* 用来验证MQTT服务端的RSA根证书 */
                cred.x509_server_cert_len = strlen(ali_ca_cert);     /* 用来验证MQTT服务端的RSA根证书长度 */
                
                aiot_download_setopt(hdl, AIOT_DLOPT_NETWORK_CRED, (void *)(&cred));
                aiot_download_setopt(hdl, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
                aiot_download_setopt(hdl, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
                aiot_download_setopt(hdl, AIOT_DLOPT_RECV_HANDLER, (void *)(http_download_recv_handler));
                aiot_download_setopt(hdl, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buf_len));
                
                aiot_download_setopt(hdl, AIOT_DLOPT_USERDATA, (void *)(ota));
                h->ota_proto = AIOT_OTA_PROTOCOL_HTTPS;
            }
            else if(ota_msg->task_desc->protocol_type == AIOT_OTA_PROTOCOL_MQTT) {
                hdl = aiot_mqtt_download_init();
                if(!hdl) {
                    return;
                }
                
                U32 request_size = ONCE_PKT_LEN;
                aiot_mqtt_download_setopt(hdl, AIOT_MDOPT_TASK_DESC, ota_msg->task_desc);
                aiot_mqtt_download_setopt(hdl, AIOT_MDOPT_DATA_REQUEST_SIZE, &request_size);
                aiot_mqtt_download_setopt(hdl, AIOT_MDOPT_RECV_HANDLE, (void *)(mqtt_download_recv_handler));

                aiot_mqtt_download_setopt(hdl, AIOT_MDOPT_USERDATA, (void *)(ota));
                h->ota_proto = AIOT_OTA_PROTOCOL_MQTT;
            }
            else {
                return;
            }
            
            h->hdl = hdl;
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
    
    aiot_dm_setopt(h->hdm, AIOT_DMOPT_USERDATA,  h);
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
    
    aiot_ntp_setopt(h->hntp, AIOT_NTPOPT_USERDATA, h);
    aiot_ntp_setopt(h->hntp, AIOT_NTPOPT_MQTT_HANDLE, h->hmq);
    aiot_ntp_setopt(h->hntp, AIOT_NTPOPT_TIME_ZONE, (int8_t *)&time_zone);
    
    aiot_ntp_setopt(h->hntp, AIOT_NTPOPT_RECV_HANDLER, (void *)ntp_recv_handler);
    aiot_ntp_setopt(h->hntp, AIOT_NTPOPT_EVENT_HANDLER, (void *)ntp_event_handler);
#endif
    
    return 0;
}
static void mqtt_ota_init(mqtt_handle_t *h)
{
    h->hota = aiot_ota_init();
    if (h->hota==NULL) {
        return;
    }
    
    aiot_ota_setopt(h->hota, AIOT_OTAOPT_USERDATA, h);
    aiot_ota_setopt(h->hota, AIOT_OTAOPT_MQTT_HANDLE, h->hmq);
    aiot_ota_setopt(h->hota, AIOT_OTAOPT_RECV_HANDLER, ota_recv_handler);
}

static void mqtt_ota_download(mqtt_handle_t *h)
{
    int r=0,quit=0;
    
    if(!h->hdl) {
        return;
    }
    
    if(h->ota_proto == AIOT_OTA_PROTOCOL_HTTPS) {
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
        else {
            //LOGD("___ aiot_download_recv -0x%03x\n", -r);
        }

        U32 timeout_ms = 100;
        aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);
        if (r==STATE_DOWNLOAD_FINISHED) {
            LOGD("download completed\n");
            aiot_download_deinit(&h->hdl);
            h->hdl = NULL;
            
            timeout_ms = 5000;
            aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);
        }
    }
    else if(h->ota_proto == AIOT_OTA_PROTOCOL_MQTT) {
        r = aiot_mqtt_download_process(h->hdl);
        
        if(STATE_MQTT_DOWNLOAD_SUCCESS==r) {
            LOGD("mqtt download success \n");
            quit = 1;
        } else if(STATE_MQTT_DOWNLOAD_FAILED_RECVERROR == r
                  || STATE_MQTT_DOWNLOAD_FAILED_TIMEOUT == r
                  || STATE_MQTT_DOWNLOAD_FAILED_MISMATCH == r) {
            LOGE("mqtt download failed, -0x%x\n", -r);
            quit = 1;
        }
        
        if(quit) {
            aiot_mqtt_download_deinit(&h->hdl);
            h->hdl = NULL;
        }
    }
}



static void start_thread(void *p)
{
    if(!proc_thread_running && !proc_thread_running) {
        LOGD("___ mqtt start threads\n");
        proc_thread_running = 1;
        recv_thread_running = 1;
        
        start_task_simp(mqtt_proc_thread, 4*KB, p, &proc_thread_id);
        start_task_simp(mqtt_recv_thread, 4*KB, p, &recv_thread_id);
    }
}
static void stop_thread(void)
{
    proc_thread_running = 0;
    recv_thread_running = 0;
    
    LOGD("___ mqtt stop threads\n");
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
        
        if(h->flags.conn==-1) {
            mqtt_reconn(h);
        }
        
        
        period = (h->flags.ntp?3600:2);
        interval = rtc2_get_timestamp_s()-h->ntime;
        if(interval>=period) {
            r = aiot_ntp_send_request(h->hntp);
            
            //LOGD("___ ntp_request: %d, ntpflag: %d, interval: %d, period: %d\n", r, mqttFlag.ntp, interval, period);
            h->ntime = rtc2_get_timestamp_s();
        }
        
        osDelay(1);
    }
    
    osThreadExit();
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
static void mqtt_recv_thread(void *arg)
{
    int r;
    int32_t res = STATE_SUCCESS;
    mqtt_handle_t *h=(mqtt_handle_t*)arg;

    while (recv_thread_running) {
        res = aiot_mqtt_recv(h->hmq);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            
            if(res==STATE_SYS_DEPEND_NWK_CLOSED) {
                osDelay(1);
            } 
        }
        
        mqtt_ota_download(h);
    }
    
    osThreadExit();
}



static int mqtt_reconn(handle_t hconn)
{
    int r;
    mqtt_handle_t *h=(mqtt_handle_t*)hconn;
    
    if(!h) {
        return -1;
    }
    
    aiot_mqtt_disconnect(h->hmq);
    r = aiot_mqtt_connect(h->hmq);
    
    return r;
}

static int mqtt_req(handle_t hconn, int req)
{
    int r=0;
    char tmp[100];
    char payload[100];
    mqtt_handle_t *h=(mqtt_handle_t*)hconn;
    conn_handle_t *conn=(conn_handle_t*)h->userdata;
    plat_para_t *plat=&conn->para.para->para.plat;
    
    if(req&MQTT_REQ_CFG) {
        char *request1 = "{\"id\":%d,\"params\":{\"configScope\":\"product\",\"getType\":\"file\"},\"method\":\"thing.config.get\"}";
        sprintf(tmp, "/sys/%s/%s/thing/config/get", plat->prdKey, plat->devKey);
        sprintf(payload, request1, allPara.sys.devInfo.devID);
        r = aiot_mqtt_pub(h->hmq, tmp, (U8*)payload, strlen(payload), 0);
    }
    
    if(req==MQTT_REQ_FW) {
        char *request2 = "{\"id\":%d,\"version\":\"1.0\",\"params\":{\"module\":\"MCU\"},\"method\":\"thing.ota.firmware.get\"}";
        sprintf(tmp, "/sys/%s/%s/thing/ota/firmware/get", plat->prdKey, plat->devKey);
        sprintf(payload, request2, allPara.sys.devInfo.devID);
        r = aiot_mqtt_pub(h->hmq, tmp, (U8*)payload, strlen(payload), 0);
    }
    
    return r;
}
static int mqtt_subx(handle_t hconn, void *para)
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
    mqtt_handle_t *h=(mqtt_handle_t*)userdata;
    conn_handle_t *conn=(conn_handle_t*)h->userdata;
    
    switch (packet->type) {

        case AIOT_MQTTRECV_PUB: {
            if(conn && conn->para.callback) {
                conn->para.callback(conn, NULL, 0, packet->data.pub.payload, packet->data.pub.payload_len, 0);
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
    smp_para_t *smp=paras_get_smp();
    
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
        
        aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_USERDATA, h);
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
        //r = aiot_mqtt_connect_v5(h->hmq, NULL, NULL);
        if(r) {
            aiot_dm_deinit(&h->hdm);
            aiot_ntp_deinit(&h->hntp);
            
            aiot_mqtt_deinit(h->hmq);
            free(h);
            return NULL;
        }
        aiot_ota_report_version(h->hota, FW_VERSION);
        
        mqtt_subx(h, NULL);
        
        if(smp->pwrmode==PWR_PERIOD_PWRDN) {
            mqtt_req(h, MQTT_REQ_CFG|MQTT_REQ_FW);
        }
        
        start_thread(h);
    }
    
    return h;
}

static int mqtt_ali_reconn(handle_t hconn)
{
    return mqtt_reconn(hconn);
}


static int mqtt_ali_disconn(handle_t hconn)
{
    int r;
    mqtt_handle_t *h=(mqtt_handle_t*)hconn;
    
    if(!h) {
        return -1;
    }
    
    stop_thread();
    
    aiot_mqtt_disconnect(h->hmq);
    r = aiot_mqtt_deinit(&h->hmq);
    if (r < STATE_SUCCESS) {
        LOGE("aiot_mqtt_deinit failed: -0x%04X\n", -r);
        return -1;
    }
    
    aiot_dm_deinit(&h->hdm);
    
#ifdef USE_NTP
    aiot_ntp_deinit(&h->hntp);
#endif
    
    
    
    return 0;
}


static int mqtt_ali_sub(handle_t hconn, void *para)
{
    return mqtt_subx(hconn, para);
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

    if(!h || !para || !data || !len || h->flags.conn<=0) {
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


static int mqtt_ali_req(handle_t hconn, int req)
{
    return mqtt_req(hconn, req);
}



mqtt_fn_t mqtt_ali_fn={
    mqtt_ali_init,
    mqtt_ali_deinit,
    
    mqtt_ali_conn,
    mqtt_ali_disconn,
    mqtt_ali_reconn,
    mqtt_ali_sub,
    mqtt_ali_pub,
    mqtt_ali_req,
};



#endif


