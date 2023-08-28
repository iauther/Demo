#include "task.h"
#include "dal.h"
#include "log.h"
#include "rtc.h"
#include "paras.h"
#include "aiot_dm_api.h"
#include "aiot_ntp_api.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"


#define USE_NTP

typedef struct {
    U8  hw;
    U8  init;
    S8  conn;
    U8  ntp;
}mqtt_flag_t;


#ifdef OS_KERNEL

static mqtt_flag_t mqttFlag={0};
static void *mqtt_handle=NULL;
static void *dm_handle=NULL;
static void *ntp_handle=NULL;
static int mqtt_sub(void);
//static int mqtt_pub(void);

static U8 proc_thread_running=0;
static U8 recv_thread_running=0;
static osThreadId_t proc_thread_id=NULL;
static osThreadId_t recv_thread_id=NULL;
static void mqtt_proc_thread(void *args);
static void mqtt_recv_thread(void *args);
static int mqtt_dm_init(void);
static int mqtt_ntp_init(void);
extern int32_t at_hal_init(void);


int32_t demo_state_logcb(int32_t code, char *message)
{
    LOGD("%s", message);
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
            
            //api_cap_start(STAT_CAP);
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
            mqttFlag.conn = -1;
            /* TODO: 处理SDK被动断连, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        default: {

        }
    }
}

/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
static void mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            LOGD("heartbeat response\n");
            /* TODO: 处理服务器对心跳的回应, 一般不处理 */
        }
        break;

        case AIOT_MQTTRECV_SUB_ACK: {
            LOGD("suback, res: -0x%04X, packet id: %d, max qos: %d\n",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
            /* TODO: 处理服务器对订阅请求的回应, 一般不处理 */
        }
        break;

        case AIOT_MQTTRECV_PUB: {
            LOGD("pub, qos: %d, topic: %.*s\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            LOGD("pub, payload: %.*s\n", packet->data.pub.payload_len, packet->data.pub.payload);
            /* TODO: 处理服务器下发的业务报文 */
        }
        break;

        case AIOT_MQTTRECV_PUB_ACK: {
            LOGD("puback, packet id: %d\n", packet->data.pub_ack.packet_id);
            /* TODO: 处理服务器对QoS1上报消息的回应, 一般不处理 */
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
                date_time_t dt;
                
                dt.date.year = packet->data.local_time.year;
                dt.date.mon = packet->data.local_time.mon;
                dt.date.day = packet->data.local_time.day;
                dt.time.hour = packet->data.local_time.hour;
                dt.time.min = packet->data.local_time.min;
                dt.time.sec = packet->data.local_time.sec;
                
                rtcx_write_time(&dt);
                mqttFlag.ntp = 1;
            }       
        }
        break;

        default: {

        }
    }
}


////////////////////////////////////////////////////////////

static int start_thread(U8 flag)
{
    if(flag) {
        proc_thread_running = 1;
        recv_thread_running = 1;
        
        LOGD("___ mqtt start proc and recv thread\n");
        
        task_simp_new(mqtt_proc_thread, 1024, mqtt_handle, &proc_thread_id);
        task_simp_new(mqtt_recv_thread, 1024, mqtt_handle, &recv_thread_id);
    }
    else {
        proc_thread_running = 0;
        recv_thread_running = 0;
        
        LOGD("___ mqtt stop proc and recv thread\n");
        
        osDelay(2);
        
        osThreadJoin(proc_thread_id);
        osThreadJoin(recv_thread_id);
    }
    
    return 0;
}
/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
static void mqtt_proc_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (proc_thread_running) {
        res = aiot_mqtt_process(args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        osDelay(1);
    }
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
static void mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (recv_thread_running) {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            osDelay(1);
        }
    }
}


static int mqtt_init(void)
{
    int i,r;
    int32_t res = STATE_SUCCESS;
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */
    net_para_t *netPara=&allPara.usr.net;
     
    if(mqttFlag.conn!=0) {
        return 0;
    }
    
    extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    aiot_state_set_logcb(demo_state_logcb);
    
    if(!mqttFlag.hw) {
        LOGD("___ at_hal_init\n");
        
        r = at_hal_init();
        if(r < 0) {
            LOGE("___ at_hal_init failed\r\n");
            return -1;
        }
        else {
            mqttFlag.hw = 1;
        }
        
        LOGD("___ at_hal_init ok\n");
    }
    
    if(!mqttFlag.init) {
        LOGD("___ aiot_mqtt_init\n");
        
        mqtt_handle = aiot_mqtt_init();
        if(!mqtt_handle) {
            LOGE("___ aiot_mqtt_init failed!\n");
            return -1;
        }
        mqttFlag.init = 1;
        
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
        
        aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)netPara->host);
        aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&netPara->port);
        aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)netPara->prdKey);
        aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)netPara->devKey);
        aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)netPara->devSecret);
        aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);

        aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)mqtt_default_recv_handler);
        aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)mqtt_event_handler);
        
        LOGD("___ aiot_mqtt_init ok\n");
    }
    
    if(!mqttFlag.conn) {
        LOGD("___ aiot_mqtt_connect ...\n");
        
        r = aiot_mqtt_connect(mqtt_handle);
        if (r<0) {
            return -1;
        }
        
        LOGD("___ aiot_mqtt_connect ok\n");
    }
    
    mqtt_dm_init();
    mqtt_ntp_init();
    
    start_thread(1);
    
    return 0;
}
static int mqtt_deinit(void)
{
    int32_t res = STATE_SUCCESS;
    
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        LOGE("aiot_mqtt_disconnect failed: -0x%04X\n", -res);
        return -1;
    }

    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        LOGE("aiot_mqtt_deinit failed: -0x%04X\n", -res);
        return -1;
    }
    
    aiot_dm_deinit(&dm_handle);
    
#ifdef USE_NTP
    aiot_ntp_deinit(&ntp_handle);
#endif
    
    
    start_thread(0);
    
    return 0;
}




static int mqtt_sub(void)
{
    int32_t res=STATE_SUCCESS;
    
    if(!mqttFlag.conn) {
        return -1;
    }
    
    char *sub_topic = "/sys/izenN4JXpJs/${YourDeviceName}/thing/event/+/post_reply";

    res = aiot_mqtt_sub(mqtt_handle, sub_topic, NULL, 1, NULL);
    if (res < 0) {
        LOGE("aiot_mqtt_sub failed, res: -0x%04X\n", -res);
        return -1;
    }
    
    return 0;
}
static char mqttPayload[1000];
static char mqttTopic[100];
static char mqttTemp[EV_NUM][200];
int mqtt_pub_str(upload_data_t *up)
{
    int32_t i,r;
    char topic[100];
    char tmp[80];
    char sid[20];
    net_para_t *netPara=&allPara.usr.net;
    
    if(!mqttFlag.conn) {
        return -1;
    }
    
    sprintf(sid, "?%d", up->id);
    sprintf(mqttTopic, "/sys/%s/%s/thing/event/property/batch/post", netPara->prdKey, netPara->devKey);
    sprintf(mqttPayload, "{\"method\":\"thing.event.property.batch.post\",\"version\":\"1.0\",\"id\":%s,\"params\":{\"ev_data:channelid\":%d,\"ev_data:sensorid\":%d,\"ev_data:signal\":%0.2f,", sid, up->ch, up->ss, up->sig);
    
    strcpy(mqttTemp[EV_RMS], "\"ev_data:rms\":[");
    strcpy(mqttTemp[EV_AMP], "\"ev_data:amp\":[");
    strcpy(mqttTemp[EV_ASL], "\"ev_data:asl\":[");
    strcpy(mqttTemp[EV_ENE], "\"ev_data:ene\":[");
    
#if 1
    //////////////////////////////////////
    for(i=0; i<up->cnt; i++) {
        
        ev_data_t *ev=&up->ev[i];
        
        sprintf(tmp, "{\"value\":%2.4f, \"time\":%lld}", ev->rms, ev->time);
        if(i<up->cnt-1) strcat(tmp, ",");
        strcat(mqttTemp[EV_RMS], tmp);

        sprintf(tmp, "{\"value\":%2.4f, \"time\":%lld}", ev->amp, ev->time);
        if(i<up->cnt-1) strcat(tmp, ",");
        strcat(mqttTemp[EV_AMP], tmp);
        
        sprintf(tmp, "{\"value\":%2.4f, \"time\":%lld}", ev->asl, ev->time);
        if(i<up->cnt-1) strcat(tmp, ",");
        strcat(mqttTemp[EV_ASL], tmp);
        
        sprintf(tmp, "{\"value\":%2.4f, \"time\":%lld}", ev->pwr, ev->time);
        if(i<up->cnt-1) strcat(tmp, ",");
        strcat(mqttTemp[EV_ENE], tmp);
    }
#endif

    strcat(mqttTemp[EV_RMS], "],");
    strcat(mqttTemp[EV_AMP], "],");
    strcat(mqttTemp[EV_ASL], "],");
    strcat(mqttTemp[EV_ENE], "]");
    
    strcat(mqttPayload, mqttTemp[EV_RMS]);
    strcat(mqttPayload, mqttTemp[EV_AMP]);
    strcat(mqttPayload, mqttTemp[EV_ASL]);
    strcat(mqttPayload, mqttTemp[EV_ENE]);    
    
    strcat(mqttPayload, "}}");
    
    r = aiot_mqtt_pub(mqtt_handle, mqttTopic, (uint8_t *)mqttPayload, (uint32_t)strlen(mqttPayload), 1);
    if (r<0) {
        LOGE("aiot_mqtt_pub failed, r: -0x%04X\n", -r);
        return -1;
    }
    
    return 0;
}
int mqtt_pub_raw(upload_data_t *up)
{
    int r,tlen=0;
    aiot_dm_msg_t msg;
    net_para_t *netPara=&allPara.usr.net;

    if(!mqttFlag.conn) {
        return -1;
    }
    
    tlen = sizeof(upload_data_t)+sizeof(ev_data_t)*up->cnt;
    
    memset(&msg, 0, sizeof(aiot_dm_msg_t));
    msg.type = AIOT_DMMSG_RAW_DATA;
    msg.data.raw_data.data = (U8*)up;
    msg.data.raw_data.data_len = tlen;

    r = aiot_dm_send(dm_handle, &msg);
    
    return r;
}
int mqtt_is_connected(void)
{
    return (mqttFlag.conn>0 && mqttFlag.ntp);
}

/////////////////////////////////////
static int mqtt_dm_init(void)
{
    uint8_t post_reply = 0;
    
    dm_handle = aiot_dm_init();
    if (dm_handle == NULL) {
        LOGE("aiot_dm_init failed\n");
        return -1;
    }
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_MQTT_HANDLE, mqtt_handle);
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_RECV_HANDLER, (void *)dm_recv_handler);
    
    //配置是云端否需要回复post_reply给设备. 如果为1, 表示需要云端回复, 否则表示不回复
    aiot_dm_setopt(dm_handle, AIOT_DMOPT_POST_REPLY, (void *)&post_reply);
    
    return 0;
}
static int mqtt_ntp_init(void)
{
    int res=0;
    int8_t time_zone = 8;
    
#ifdef USE_NTP
    ntp_handle = aiot_ntp_init();
    if (ntp_handle == NULL) {
        LOGE("aiot_ntp_init failed\n");
        return -1;
    }

    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_MQTT_HANDLE, mqtt_handle);
    if (res < STATE_SUCCESS) {
        LOGE("aiot_ntp_setopt AIOT_NTPOPT_MQTT_HANDLE failed, res: -0x%04X\n", -res);
        aiot_ntp_deinit(&ntp_handle);
        return -1;
    }

    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_TIME_ZONE, (int8_t *)&time_zone);
    if (res < STATE_SUCCESS) {
        LOGE("aiot_ntp_setopt AIOT_NTPOPT_TIME_ZONE failed, res: -0x%04X\n", -res);
        aiot_ntp_deinit(&ntp_handle);
        return -1;
    }

    /* TODO: NTP消息回应从云端到达设备时, 会进入此处设置的回调函数 */
    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_RECV_HANDLER, (void *)ntp_recv_handler);
    if (res < STATE_SUCCESS) {
        LOGE("aiot_ntp_setopt AIOT_NTPOPT_RECV_HANDLER failed, res: -0x%04X\n", -res);
        aiot_ntp_deinit(&ntp_handle);
        return -1;
    }

    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_EVENT_HANDLER, (void *)ntp_event_handler);
    if (res < STATE_SUCCESS) {
        LOGE("aiot_ntp_setopt AIOT_NTPOPT_EVENT_HANDLER failed, res: -0x%04X\n", -res);
        aiot_ntp_deinit(&ntp_handle);
        return -1;
    }
#endif
    
    return 0;
}
static int mqtt_ntp_request(void)
{
    int r=0;
    
    if(!mqttFlag.conn) {
        return -1;
    }
    
#ifdef USE_NTP
    r = aiot_ntp_send_request(ntp_handle);
#endif
    
    return r;
}





void task_mqtt_fn(void *arg)
{
    int i,r,mode;
    U32 count=0;
    U8  err;
    
    LOGD("___ task_mqtt running\n");
    
    while(1) {
        
        mqtt_init();
        
        if(mqttFlag.conn>0) {
            mode = mqttFlag.ntp?3600:1;
            if(count%mode==0) {  //1小时同步一次
                mqtt_ntp_request();
            }
        }
        
        osDelay(1000);
        //osThreadYield();
        
        count++;
    }
    
    mqtt_deinit();
}
#endif


