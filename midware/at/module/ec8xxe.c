/*----------------------------------------------------------------------------
 * Tencent is pleased to support the open source community by making TencentOS
 * available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
 * If you have downloaded a copy of the TencentOS binary from Tencent, please
 * note that the TencentOS binary is licensed under the BSD 3-Clause License.
 *
 * If you have downloaded a copy of the TencentOS source code from Tencent,
 * please note that TencentOS source code is licensed under the BSD 3-Clause
 * License, except for the third-party components listed below which are
 * subject to different license terms. Your integration of TencentOS into your
 * own projects may require compliance with the BSD 3-Clause License, as well
 * as the other licenses applicable to the third-party components included
 * within TencentOS.
 *---------------------------------------------------------------------------*/

#include "ec800e.h"
#include "at.h"
#include "at_hal.h"
#include "at_port.h"
#include "at_module.h"

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct ip_addr_st {
    uint8_t seg1;
    uint8_t seg2;
    uint8_t seg3;
    uint8_t seg4;
}ip_addr_t;

at_agent_t ec800e_agent;
static ip_addr_t domain_parser_addr = {0};
static k_sem_t domain_parser_sem;
static k_stack_t ec800e_at_parse_task_stk[AT_PARSER_TASK_STACK_SIZE];

#define AT_AGENT    ((at_agent_t *)&ec800e_agent)


static int ec800e_check_ready(void)
{
    at_echo_t echo;
    int try = 0;

    while (try++ < 10) {
        at_echo_create(&echo, NULL, 0, NULL);
        at_cmd_exec(AT_AGENT, &echo, 1000, "AT\r\n");
        if (echo.status == AT_ECHO_STATUS_OK) {
            return 0;
        }
    }

    return -1;
}

static int ec800e_echo_close(void)
{
    at_echo_t echo;
    int try = 0;

    at_echo_create(&echo, NULL, 0, NULL);

    while (try++ < 10) {
        at_cmd_exec(AT_AGENT, &echo, 1000, "ATE0\r\n");
        if (echo.status == AT_ECHO_STATUS_OK) {
            return 0;
        }
    }

    return -1;
}
static int ec800e_sim_card_check(void)
{
    at_echo_t echo;
    int try = 0;
    char echo_buffer[32];

    at_echo_create(&echo, echo_buffer, sizeof(echo_buffer), NULL);
    while (try++ < 10) {
        at_cmd_exec(AT_AGENT, &echo, 1000, "AT+CPIN?\r\n");
        if (strstr(echo_buffer, "READY")) {
            return 0;
        }
        AT_PORT->delay_ms(2000);
    }

    return -1;
}

static int ec800e_signal_quality_check(void)
{
    int rssi, ber;
    at_echo_t echo;
    char echo_buffer[32], *str;
    int try = 0;

    at_echo_create(&echo, echo_buffer, sizeof(echo_buffer), NULL);
    while (try++ < 10) {
        at_cmd_exec(AT_AGENT, &echo, 1000, "AT+CSQ\r\n");
        if (echo.status != AT_ECHO_STATUS_OK) {
            return -1;
        }

        str = strstr(echo.buffer, "+CSQ:");
        if (!str) {
            return -1;
        }

        sscanf(str, "+CSQ:%d,%d", &rssi, &ber);
        if (rssi != 99) {
            return 0;
        }
        sleep_ms(2000);
    }

    return -1;
}
static int ec800e_gsm_network_check(void)
{
    int n, stat;
    at_echo_t echo;
    char echo_buffer[32], *str;
    int try = 0;

    at_echo_create(&echo, echo_buffer, sizeof(echo_buffer), NULL);
    while (try++ < 10) {
        at_cmd_exec(AT_AGENT, &echo, 1000, "AT+CREG?\r\n");
        if (echo.status != AT_ECHO_STATUS_OK) {
            return -1;
        }

        str = strstr(echo.buffer, "+CREG:");
        if (!str) {
            return -1;
        }
        sscanf(str, "+CREG:%d,%d", &n, &stat);
        if (stat == 1) {
            return 0;
        }
        AT_PORT->delay_ms(2000);
    }

    return -1;
}

static int ec800e_gprs_network_check(void)
{
    int n, stat;
    at_echo_t echo;
    char echo_buffer[32], *str;
    int try = 0;

    at_echo_create(&echo, echo_buffer, sizeof(echo_buffer), NULL);
    while (try++ < 10) {
        at_cmd_exec(AT_AGENT, &echo, 1000, "AT+CGREG?\r\n");
        if (echo.status != AT_ECHO_STATUS_OK) {
            return -1;
        }

        str = strstr(echo.buffer, "+CGREG:");
        if (!str) {
            return -1;
        }
        sscanf(str, "+CGREG:%d,%d", &n, &stat);
        if (stat == 1) {
            return 0;
        }
        AT_PORT->delay_ms(2000);
    }

    return -1;
}

static int ec800e_close_apn(void)
{
    at_echo_t echo;

    at_echo_create(&echo, NULL, 0, NULL);
    at_cmd_exec(AT_AGENT, &echo, 3000, "AT+QIDEACT=1\r\n");
    if (echo.status == AT_ECHO_STATUS_OK) {
        return 0;
    }

    return -1;
}

static int ec800e_set_apn(void)
{
    at_echo_t echo;

    at_echo_create(&echo, NULL, 0, NULL);
    at_cmd_exec(AT_AGENT, &echo, 1000, "AT+QICSGP=1,1,\"CMNET\"\r\n");
    if (echo.status != AT_ECHO_STATUS_OK) {
        return -1;
    }

    at_cmd_exec(AT_AGENT, &echo, 3000, "AT+QIACT=1\r\n");
    if (echo.status != AT_ECHO_STATUS_OK) {
        return -1;
    }

    return 0;
}

static int ec800e_init(void)
{
    AT_PORT->print("Init ec800e ...\n" );

    if (ec800e_check_ready() != 0) {
        AT_PORT->print("wait module ready timeout, please check your module\n");
        return -1;
    }

    if (ec800e_echo_close() != 0) {
        AT_PORT->print("echo close failed,please check your module\n");
        return -1;
    }

    if(ec800e_sim_card_check() != 0) {
        AT_PORT->print("sim card check failed,please insert your card\n");
        return -1;
    }

    if (ec800e_signal_quality_check() != 0) {
        AT_PORT->print("signal quality check status failed\n");
        return -1;
    }

    if(ec800e_gsm_network_check() != 0) {
        AT_PORT->print("GSM network register status check fail\n");
        return -1;
    }

    if(ec800e_gprs_network_check() != 0) {
        AT_PORT->print("GPRS network register status check fail\n");
        return -1;
    }

    if(ec800e_close_apn() != 0) {
        AT_PORT->print("close apn failed\n");
        return -1;
    }

    if (ec800e_set_apn() != 0) {
        AT_PORT->print("apn set FAILED\n");
        return -1;
    }

    AT_PORT->print("Init ec800e ok\n" );
    return 0;
}

static int ec800e_connect_establish(int id, at_proto_t proto)
{
    at_echo_t echo;
    char except_str[16];
    char echo_buffer[64];
    char *query_result_str = NULL;
    char *remote_ip = NULL;
    char *remote_port = NULL;

    at_echo_create(&echo, echo_buffer, sizeof(echo_buffer), NULL);
    at_cmd_exec(AT_AGENT, &echo, 1000, "AT+QISTATE=1,%d\r\n", id);
    if (echo.status != AT_ECHO_STATUS_OK) {
        AT_PORT->print("query socket %d state fail\r\n", id);
        return -1;
    }

    sAT_PORT->print(except_str, "+QISTATE: %d", id);
    query_result_str = strstr(echo_buffer, except_str);
    if (query_result_str) {
        AT_PORT->print("socket %d established on module already\r\n", id);
        at_echo_create(&echo, NULL, 0, NULL);
        at_cmd_exec(AT_AGENT, &echo, 1000, "AT+QICLOSE=%d\r\n", id);
    }

    memset(except_str, 0, sizeof(except_str));
    sAT_PORT->print(except_str, "+QIOPEN: %d,0", id);

    remote_ip = (char*)at_channel_ip_get(AT_AGENT, id);
    remote_port = (char*)at_channel_port_get(AT_AGENT, id);
    if (!remote_ip || !remote_port) {
        return -2;
    }

    at_echo_create(&echo, NULL, 0, except_str);
    at_cmd_exec_until(AT_AGENT, &echo, 4000, "AT+QIOPEN=1,%d,\"%s\",\"%s\",%d,0,1\r\n",
                        id, proto == TOS_at_PROTO_UDP ? "UDP" : "TCP", remote_ip, atoi(remote_port));
    if (echo.status != AT_ECHO_STATUS_EXPECT) {
        AT_PORT->print("establish socket %d on module fail\r\n", id);
        return -3;
    }

    return 0;
}

static int ec800e_connect(const char *ip, const char *port, at_proto_t proto)
{
    int id;

    id = at_channel_alloc(AT_AGENT, ip, port);
    if (id == -1) {
        AT_PORT->print("at channel alloc fail\r\n");
        return -1;
    }

    if (ec800e_connect_establish(id, proto) < 0) {
        at_channel_free(AT_AGENT, id);
        return -2;
    }

    return id;
}

static int ec800e_connect_with_size(const char *ip, const char *port, at_proto_t proto, size_t socket_buffer_size)
{
    int id;

    id = at_channel_alloc_with_size(AT_AGENT, ip, port, socket_buffer_size);
    if (id == -1) {
        AT_PORT->print("at channel alloc fail\r\n");
        return -1;
    }

    if (ec800e_connect_establish(id, proto) < 0) {
        at_channel_free(AT_AGENT, id);
        return -2;
    }

    return id;
}

static int ec800e_recv_timeout(int id, void *buf, size_t len, uint32_t timeout)
{
    return at_channel_read_timed(AT_AGENT, id, buf, len, timeout);
}

static int ec800e_recv(int id, void *buf, size_t len)
{
    return ec800e_recv_timeout(id, buf, len, (uint32_t)4000);
}

int ec800e_send(int id, const void *buf, size_t len)
{
    at_echo_t echo;

    if (!at_channel_is_working(AT_AGENT, id)) {
        return -1;
    }

    at_echo_create(&echo, NULL, 0, ">");

    at_cmd_exec_until(AT_AGENT, &echo, 1000, "AT+QISEND=%d,%d\r\n", id, len);

    if (echo.status != AT_ECHO_STATUS_EXPECT) {
        return -1;
    }

    at_echo_create(&echo, NULL, 0, "SEND OK");
    at_raw_data_send_until(AT_AGENT, &echo, 10000, (uint8_t *)buf, len);
    if (echo.status != AT_ECHO_STATUS_EXPECT) {
        return -1;
    }

    return len;
}

int ec800e_recvfrom_timeout(int id, void *buf, size_t len, uint32_t timeout)
{
    return at_channel_read_timed(AT_AGENT, id, buf, len, timeout);
}

int ec800e_recvfrom(int id, void *buf, size_t len)
{
    return ec800e_recvfrom_timeout(id, buf, len, (uint32_t)4000);
}

int ec800e_sendto(int id, char *ip, char *port, const void *buf, size_t len)
{
    at_echo_t echo;

    at_echo_create(&echo, NULL, 0, ">");
    at_cmd_exec_until(AT_AGENT, &echo, 1000, "AT+QISEND=%d,%d\r\n", id, len);

    if (echo.status != AT_ECHO_STATUS_EXPECT) {
        return -1;
    }

    at_echo_create(&echo, NULL, 0, "SEND OK");
    at_raw_data_send(AT_AGENT, &echo, 1000, (uint8_t *)buf, len);
    if (echo.status != AT_ECHO_STATUS_EXPECT) {
        return -1;
    }

    return len;
}

static void ec800e_transparent_mode_exit(void)
{
    at_echo_t echo;

    at_echo_create(&echo, NULL, 0, NULL);
    at_cmd_exec(AT_AGENT, &echo, 500, "+++");
}

static int ec800e_close(int id)
{
    at_echo_t echo;

    ec800e_transparent_mode_exit();

    at_echo_create(&echo, NULL, 0, NULL);
    at_cmd_exec(AT_AGENT, &echo, 1000, "AT+QICLOSE=%d\r\n", id);

    at_channel_free(AT_AGENT, id);

    return 0;
}

static int ec800e_parse_domain(const char *host_name, char *host_ip, size_t host_ip_len)
{
    at_echo_t echo;
    char echo_buffer[128];

    sem_create_max(&domain_parser_sem, 0, 1);

    at_echo_create(&echo, echo_buffer, sizeof(echo_buffer), NULL);
    at_cmd_exec(AT_AGENT, &echo, 2000, "AT+QIDNSGIP=1,\"%s\"\r\n", host_name);

    if (echo.status != AT_ECHO_STATUS_OK) {
        return -1;
    }

    sem_pend(&domain_parser_sem, TOS_TIME_FOREVER);
    snAT_PORT->print(host_ip, host_ip_len, "%d.%d.%d.%d", domain_parser_addr.seg1, domain_parser_addr.seg2, domain_parser_addr.seg3, domain_parser_addr.seg4);
    host_ip[host_ip_len - 1] = '\0';

    AT_PORT->print("GOT IP: %s\n", host_ip);

    return 0;
}

__STATIC__ void ec800e_incoming_data_process(void)
{
    uint8_t data;
    int channel_id = 0, data_len = 0, read_len;
    uint8_t buffer[128];

    /*
		+QIURC: "recv",<sockid>,<datalen>
		<data content>
    */

    while (1) {
        if (at_uart_read(AT_AGENT, &data, 1) != 1) {
            return;
        }
        if (data == ',') {
            break;
        }
        channel_id = channel_id * 10 + (data - '0');
    }

    while (1) {
        if (at_uart_read(AT_AGENT, &data, 1) != 1) {
            return;
        }

        if (data == '\r') {
            break;
        }
        data_len = data_len * 10 + (data - '0');
    }

    if (at_uart_read(AT_AGENT, &data, 1) != 1) {
        return;
    }

    do {
#if !defined(MIN)
#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#endif
        read_len = MIN(data_len, sizeof(buffer));
        if (at_uart_read(AT_AGENT, buffer, read_len) != read_len) {
            return;
        }

        if (at_channel_write(AT_AGENT, channel_id, buffer, read_len) <= 0) {
            return;
        }

        data_len -= read_len;
    } while (data_len > 0);

    return;
}

__STATIC__ void ec800e_domain_data_process(void)
{
    uint8_t data;

    /*
        +QIURC: "dnsgip",0,1,600

		+QIURC: "dnsgip","xxx.xxx.xxx.xxx"
    */

    if (at_uart_read(AT_AGENT, &data, 1) != 1) {
        return;
    }

    if (data == '0') {
        return;
    }

    if (data == '\"') {
        /* start parser domain */
        while (1) {
            if (at_uart_read(AT_AGENT, &data, 1) != 1) {
                return;
            }
            if (data == '.') {
                break;
            }
            domain_parser_addr.seg1 = domain_parser_addr.seg1 *10 + (data-'0');
        }
        while (1) {
            if (at_uart_read(AT_AGENT, &data, 1) != 1) {
                return;
            }
            if (data == '.') {
                break;
            }
            domain_parser_addr.seg2 = domain_parser_addr.seg2 *10 + (data-'0');
        }
        while (1) {
            if (at_uart_read(AT_AGENT, &data, 1) != 1) {
                return;
            }
            if (data == '.') {
                break;
            }
            domain_parser_addr.seg3 = domain_parser_addr.seg3 *10 + (data-'0');
        }
        while (1) {
            if (at_uart_read(AT_AGENT, &data, 1) != 1) {
                return;
            }
            if (data == '\"') {
                break;
            }
            domain_parser_addr.seg4 = domain_parser_addr.seg4 *10 + (data-'0');
        }
        g_at_port.sem_release(&domain_parser_sem);
    }
    return;

}

at_event_t ec800e_at_event[] = {
	{ "+QIURC: \"recv\",",   ec800e_incoming_data_process},
    { "+QIURC: \"dnsgip\",", ec800e_domain_data_process},
};

at_module_t at_module_ec800e = {
    .init               = ec800e_init,
    .connect            = ec800e_connect,
    .connect_with_size  = ec800e_connect_with_size,
    .send               = ec800e_send,
    .recv_timeout       = ec800e_recv_timeout,
    .recv               = ec800e_recv,
    .sendto             = ec800e_sendto,
    .recvfrom           = ec800e_recvfrom,
    .recvfrom_timeout   = ec800e_recvfrom_timeout,
    .close              = ec800e_close,
    .parse_domain       = ec800e_parse_domain,
};

int ec800e_at_init(hal_uart_port_t uart_port)
{

    if (at_init(AT_AGENT, "ec800e_at", ec800e_at_parse_task_stk,
                    uart_port, ec800e_at_event,
                        sizeof(ec800e_at_event) / sizeof(ec800e_at_event[0])) != 0) {
        return -1;
    }

    if (at_module_register(&at_module_ec800e) != 0) {
        return -1;
    }
    if (at_module_init() != 0) {
        return -1;
    }

    return 0;
}

int ec800e_at_deinit(void)
{
    int id = 0;

    for (id = 0; id < AT_DATA_CHANNEL_NUM; ++id) {
        at_module_close(id);
    }

    at_module_register_default();

    at_deinit(AT_AGENT);

    return 0;
}

