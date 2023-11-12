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
#include "mqtt.h"
#include "cfg.h"
#include "hw_at_impl.h"


#define USE_NTP

//#define MQTT_SSL
//#define DOWN_SSL



typedef struct {
    U8  hw;
    U8  init;
    S8  conn;
    U8  ntp;
}flags_t;

typedef struct {
    handle_t    hmq;
    handle_t    hdm;
    handle_t    hntp;
    handle_t    hota;
    handle_t    hdl;
    
    void        *userdata;
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
static int mqtt_ali_req_cfg(handle_t hconn);

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


/* MQTT�¼��ص�����, ����������/����/�Ͽ�ʱ������, �¼������core/aiot_mqtt_api.h */
static void mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /* SDK��Ϊ�û�������aiot_mqtt_connect()�ӿ�, ��mqtt���������������ѳɹ� */
        case AIOT_MQTTEVT_CONNECT: {
            LOGD("AIOT_MQTTEVT_CONNECT\n");
            mqttFlag.conn = 1;
            
            /* TODO: ����SDK�����ɹ�, ��������������ú�ʱ�ϳ����������� */
        }
        break;

        /* SDK��Ϊ����״������������, �Զ����������ѳɹ� */
        case AIOT_MQTTEVT_RECONNECT: {
            LOGD("AIOT_MQTTEVT_RECONNECT\n");
            mqttFlag.conn = 2;
            /* TODO: ����SDK�����ɹ�, ��������������ú�ʱ�ϳ����������� */
        }
        break;

        /* SDK��Ϊ�����״���������Ͽ�������, network�ǵײ��дʧ��, heartbeat��û�а�Ԥ�ڵõ����������Ӧ�� */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            LOGD("AIOT_MQTTEVT_DISCONNECT: %s\n", cause);
            mqttFlag.conn = 0;
            /* TODO: ����SDK��������, ��������������ú�ʱ�ϳ����������� */
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
        /* TODO: �ṹ�� aiot_ntp_recv_t{} �а�����ǰʱ����, ������ʱ�������ֵ, ������������ǽ����������� */
        case AIOT_NTPRECV_LOCAL_TIME: {
            //LOGD("___ AIOT_NTPRECV_LOCAL_TIME\n");
            {
                date_time_t dt;
                
                mqttFlag.ntp = 1;
                LOGD("___ ntp data recved!\n");
                
                dt.date.year = packet->data.local_time.year;
                dt.date.mon = packet->data.local_time.mon;
                dt.date.day = packet->data.local_time.day;
                dt.time.hour = packet->data.local_time.hour;
                dt.time.min = packet->data.local_time.min;
                dt.time.sec = packet->data.local_time.sec;
                
                
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

static void download_recv_handler(void *handle, const aiot_download_recv_t *packet, void *userdata)
{
    /* Ŀǰֻ֧�� packet->type Ϊ AIOT_DLRECV_HTTPBODY ����� */
    if (!packet || AIOT_DLRECV_HTTPBODY != packet->type) {
        return;
    }
    int32_t percent = packet->data.percent;
    uint8_t *src_buffer = packet->data.buffer;
    uint32_t src_buffer_len = packet->data.len;

    /* ��� percent Ϊ����, ˵���������հ��쳣����digestУ����� */
    if (percent < 0) {
        /* �հ��쳣����digestУ����� */
        LOGE("exception happend, percent is %d\r\n", percent);
        return;
    }
    
    //aiot_download_report_progress(handle, percent);       //���ƶ˱�����������
    //aiot_ota_report_version(ota_handle, new_version);     //�¹̼�����汾���ƶ˾ݴ��ж������ɹ�
    
    if (percent == 100) {
        
        //upgrade_info();
        
    }
    
}
static void download_thread(void *arg)
{
    int32_t r=0,req_flag=-1;
    uint32_t start=0,end=0;
    uint32_t timeout_ms = 100;
    mqtt_handle_t *h=(mqtt_handle_t*)arg;
    
    aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);
    while (1) {
        
        r= aiot_download_send_request(h->hdl);
        
        /* ��������ȡ��������Ӧ�Ĺ̼����� */
        r = aiot_download_recv(h->hdl);

        /* �̼�ȫ��������ʱ, aiot_download_recv() �ķ���ֵ����� STATE_DOWNLOAD_FINISHED, �����ǵ��λ�ȡ���ֽ��� */
        if (r==STATE_DOWNLOAD_FINISHED) {
            LOGD("download completed\r\n");
            break;
        }
    }
    timeout_ms = 5000;
    aiot_mqtt_setopt(h->hmq, AIOT_MQTTOPT_RECV_TIMEOUT_MS, (void *)&timeout_ms);

    /* �������й̼��������, �������ػỰ, �߳������˳� */
    aiot_download_deinit(&h->hdl);
    h->hdl = NULL;
    
    LOGD("download thread exit\r\n");
}


static void ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    int r;
    mqtt_handle_t *h=(mqtt_handle_t*)userdata;
    
    if (NULL == ota_msg->task_desc || ota_msg->task_desc->protocol_type != AIOT_OTA_PROTOCOL_HTTPS) {
        return;
    }
    
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA:     //firmware data
        {
            
        }
        break;
        
        case AIOT_OTARECV_COTA:     //config data
        {
            uint16_t port;
            uint32_t max_buffer_len = 2048;
            aiot_sysdep_network_cred_t cred;
            
            memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
#ifdef DOWN_SSL
            port = 443;
            cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;
            cred.max_tls_fragment = 16384;
            cred.x509_server_cert = ali_ca_cert;                 /* ������֤MQTT����˵�RSA��֤�� */
            cred.x509_server_cert_len = strlen(ali_ca_cert);     /* ������֤MQTT����˵�RSA��֤�鳤�� */ 
#else
            port = 80;
            cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
#endif
            
            h->hdl = aiot_download_init();
            if(h->hdl) {
                aiot_download_setopt(h->hdl, AIOT_DLOPT_NETWORK_CRED, (void *)(&cred));
                aiot_download_setopt(h->hdl, AIOT_DLOPT_NETWORK_PORT, (void *)(&port));
                
                r = aiot_download_setopt(h->hdl, AIOT_DLOPT_TASK_DESC, (void *)(ota_msg->task_desc));
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_download_setopt 1 failed\n");
                
                r = aiot_download_setopt(h->hdl, AIOT_DLOPT_RECV_HANDLER, (void *)(download_recv_handler));
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_download_setopt 2 failed\n");
                
                r = aiot_download_setopt(h->hdl, AIOT_DLOPT_BODY_BUFFER_MAX_LEN, (void *)(&max_buffer_len));
                if(r!=STATE_SUCCESS) LOGE("_____ aiot_download_setopt 3 failed\n");
                
                if(r==STATE_SUCCESS) {
                    start_task_simp(download_thread, 1024*4, h, NULL);
                }
            }
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
    
    //�������ƶ˷���Ҫ�ظ�post_reply���豸. ���Ϊ1, ��ʾ��Ҫ�ƶ˻ظ�, �����ʾ���ظ�
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

    /* TODO: NTP��Ϣ��Ӧ���ƶ˵����豸ʱ, �����˴����õĻص����� */
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
        
        start_task_simp(mqtt_proc_thread, 2048, p, &proc_thread_id);
        start_task_simp(mqtt_recv_thread, 2048, p, &recv_thread_id);
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



/* ִ��aiot_mqtt_process���߳�, �����������ͺ�QoS1��Ϣ�ط� */
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
            
            LOGD("___ ntp_request: %d, ntpflag: %d, interval: %d, period: %d\n", r, mqttFlag.ntp, interval, period);
            ntp_time = rtc2_get_timestamp_s();
        }
        
        osDelay(1);
    }
}

/* ִ��aiot_mqtt_recv���߳�, ���������Զ������ʹӷ�������ȡMQTT��Ϣ */
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
    
    r = at_hal_boot();
    
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
    aiot_sysdep_network_cred_t cred;            /* ��ȫƾ�ݽṹ��, ���Ҫ��TLS, ����ṹ��������CA֤��Ȳ��� */
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
        cred.x509_server_cert = ali_ca_cert;                 /* ������֤MQTT����˵�RSA��֤�� */
        cred.x509_server_cert_len = strlen(ali_ca_cert);     /* ������֤MQTT����˵�RSA��֤�鳤�� */ 
        
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
        aiot_ota_report_version(h->hota, VERSION);
        
        mqtt_ali_sub(h, NULL);
        //mqtt_ali_req_cfg(h);
        
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


static int mqtt_ali_req_cfg(handle_t hconn)
{
    int r=0;
    char tmp[100];
    char payload[100];
    char *request = "{\"id\":%d,\"params\":{\"configScope\":\"product\",\"getType\":\"file\"},\"method\":\"thing.config.get\"}";
    mqtt_handle_t *h=(mqtt_handle_t*)hconn;
    conn_handle_t *conn=(conn_handle_t*)h->userdata;
    plat_para_t *plat=&conn->para.para->para.plat;
    
    sprintf(tmp, "/sys/%s/%s/thing/config/get", plat->prdKey, plat->devKey);
    sprintf(payload, request, allPara.sys.devInfo.devID);
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
    mqtt_ali_req_cfg,
    mqtt_ali_ntp_synced,
};



#endif


