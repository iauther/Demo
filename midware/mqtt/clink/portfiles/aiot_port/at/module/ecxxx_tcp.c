#if (MQTT_LIB==1)

#include "aiot_at_api.h"
#include <string.h>
#include <stdio.h>

extern stat_info_t stat_info;

/* 模块初始化命令表 */
static core_at_cmd_item_t at_ip_init_cmd_table[] = {
    {   /* 设置身份认证参数 */
        .cmd = "AT+QICSGP=1,1,\"CMNET\",\"\",\"\",1\r\n",
        .rsp = "OK",
    },
#if 1
    {   /* 去场景激活 */
        .cmd = "AT+QIDEACT=1\r\n",
        .rsp = "OK",
    },
#endif
    {   /* 场景激活 */
        .cmd = "AT+QIACT=1\r\n",
        .rsp = "OK",
    },
#if 0
    {   /* 查询场景激活 */
        .cmd = "AT+QIACT?\r\n",
        .rsp = "OK",
    },
#endif
};

/* TCP建立连接AT命令表 */
static core_at_cmd_item_t at_connect_cmd_table[] = {
    {   /* 建立TCP连接, TODO: aiot_at_nwk_connect接口会组织此AT命令 */
        .fmt = "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d,0,1\r\n",
        .rsp = "+QIOPEN",
    },
};



static char *find_chr(char *s, char c, int idx)
{
    int i=0;
    char *p=s;
    
    while(*p) {
        if(*p==c) {
            i++;
            if(i==idx) {
                return p+1;
            }
        }
        p++;
    }
    
    return NULL;
}
static at_rsp_result_t at_stat_handler(char *rsp)
{
    at_rsp_result_t r = AT_RSP_WAITING;
    int rssi = 0, ber = 0;
    int rsrp = 0, snr = 0;
    char *line = NULL;
    stat_info_t *si=&stat_info;
    
    line = strstr(rsp, "+CSQ");
    if(line && sscanf(line, "+CSQ: %d,%d", &rssi, &ber)==2) {
        if(rssi==99 || rssi==199) {
            si->rssi = -1;
        }
        else if(rssi>=0 && rssi<99){
            si->rssi = -113+rssi*2;
        }
        else if(rssi>=100 && rssi<199){
            si->rssi = -116+(rssi-100);
        }
        si->ber  = (ber==99)?-1:ber;
        r = AT_RSP_SUCCESS;
    }
    else {
        line = strstr(rsp, "+QENG");
        if(line) {
            char *s = find_chr(line, ',', 13);
            if(s && sscanf(s, "%d,%*d,%*d,%d,%*d", &rsrp, &snr)==2) {
                si->rsrp = rsrp;
                si->snr  = snr;
                    
                r = AT_RSP_SUCCESS;
            }
            else {
                r =  AT_RSP_FAILED;
            }
        }
    }

    return r;
}
static core_at_cmd_item_t at_stat_cmd_table[] = {
    {
        .cmd = "AT+CSQ\r\n",
        .rsp = "OK",
        .handler = at_stat_handler,
    }, 
    
    {
        .cmd = "AT+QENG=\"servingcell\"\r\n",
        .rsp = "OK",
        .handler = at_stat_handler,
    }, 
};

static core_at_cmd_item_t at_pwr_cmd_table[] = {
    {   //is_poweron
        .fmt = " AT+VER\r\n",
        .rsp = "OK",
    },
    
    {   //powerdown
        .fmt = "AT+QPOWD=1\r\n",
        .rsp = "POWERED DOWN",
    },
};


/* 发送数据AT命令表 */
static core_at_cmd_item_t at_send_cmd_table[] = {
    {
        .fmt = "AT+QISEND=%d,%d\r\n",
        .rsp = ">",
    },
    {
        /* 纯数据，没有格式*/
        .rsp = "SEND OK",
    },
};

/* TCP关闭连接AT命令表 */
static core_at_cmd_item_t at_disconn_cmd_table[] = {
    {   /* 建立TCP连接 */
        .fmt = "AT+QICLOSE=%d\r\n",
        .rsp = "OK",
    }
};
static core_at_recv_data_prefix at_recv = {
    .prefix = "+QIURC: \"recv\"",
    .fmt = "+QIURC: \"recv\",%d,%d\r\n",
};

at_device_t ecxxx_at_cmd = {
    .ip_init_cmd = at_ip_init_cmd_table,
    .ip_init_cmd_size = sizeof(at_ip_init_cmd_table) / sizeof(core_at_cmd_item_t),

    .open_cmd = at_connect_cmd_table,
    .open_cmd_size = sizeof(at_connect_cmd_table) / sizeof(core_at_cmd_item_t),

    .stat_cmd = at_stat_cmd_table,
    .stat_cmd_size = sizeof(at_stat_cmd_table) / sizeof(core_at_cmd_item_t),
    
    .pwr_cmd = at_pwr_cmd_table,
    .pwr_cmd_size = sizeof(at_pwr_cmd_table) / sizeof(core_at_cmd_item_t),
    
    .send_cmd = at_send_cmd_table,
    .send_cmd_size = sizeof(at_send_cmd_table) / sizeof(core_at_cmd_item_t),

    .close_cmd = at_disconn_cmd_table,
    .close_cmd_size = sizeof(at_disconn_cmd_table) / sizeof(core_at_cmd_item_t),
    
    .recv = &at_recv,
    .error_prefix = "ERROR",
};
#endif



