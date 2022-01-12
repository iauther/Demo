#ifndef __TCP232_Hx__
#define __TCP232_Hx__

#include "types.h"
#include "drv/uart.h"

typedef void (*tcp232_rx_t)(U8 *data, U16 data_len);

enum {
    AT_E=0,         //打开/关闭回显功能
    AT_Z,           //重启模块
    AT_VER,         //查询版本号
    AT_ENTM,        //退出 AT 指令模式
    AT_RELD,        //恢复出厂设置
    AT_MAC,         //查询模块 MAC
    AT_WEBU,        //设置/查询用户名和密码
    AT_WANN,        //设置/查询 WAN 口参数
    AT_DNS,         //设置/查询 DNS 服务器地址
    AT_WEBPORT,     //设置/查询网页端口号
    AT_UART,        //设置/查询串口参数
    AT_SOCK,        //设置/查询 SOCK 参数
    AT_TCPSE,       //设置/查询是否踢掉旧连接
    AT_SOCKLK,      //查询 TCP 连接状态
    AT_SOCKPORT,    //设置/查询本地端口号
    AT_RFCEN,       //设置/查询类 RFC2217 使能
    AT_PDTIME,      //查询生产时间
    
    AT_REGEN,       //设置/查询注册包机制
    AT_REGTCP,      //设置/查询注册包执行机
    AT_REGCLOUD,    //设置/查询透传云用户名和密码
    AT_REGUSR,      //设置/查询用户自定义注册包内容
    
    AT_HTPTP,       //设置/查询 Httpd Client 模式下，HTTP 的请求方式
    AT_HTPURL,      //设置/查询 Httpd Client 模式下的 URL
    AT_HTPHEAD,     //设置/查询 Httpd Client 模式下包头
    AT_HTPCHD,      //设置/查询 HTP 去包头功能
    
    AT_HEARTEN,     //设置/查询心跳包使能
    AT_HEARTTP,     //设置/查询心跳包发送方式
    AT_HEARTTM,     //设置/查询心跳包时间
    AT_HEARTDT,     //设置/查询自定义心跳包数据

    AT_SCSLINK,     //设置/查询 Socket 连接状态指示功能
    AT_CLIENTRST,   //设置/查询 TCP Client 模式连接多次失败 Reset 功能
    AT_INDEXEN,     //设置/查询 index 功能
    AT_SOCKSL,      //设置/查询短连接功能
    AT_SHORTO,      //设置/查询短连接时间
    AT_UARTCLBUF,   //设置/查询模块连接前是否清理串口缓存
    AT_RSTIM,       //设置/查询超时重启时间
    AT_MAXSK,       //设置/查询 TCP Server 连接 Client 最大值
    AT_MID,         //设置/查询模块名称
    AT_H,           //帮助

    AT_CFGTF,       //当前参数保存为用户默认参数
    AT_PING,        //主动 ping 功 
    
    AT_MAX,
};


typedef struct {
    U8      ip[30];
    U16     port;
}netaddr_t;


typedef struct {
    U8      mode[10];       //static / dhcp
    U8      dns[20];
    U8      mask[20];
    U8      gateway[20];
    
    netaddr_t cliAddr;
    netaddr_t svrAddr;
}tcp232_net_t;

typedef struct {
    U32     baudRate;
    U8      dataBits;
    U8      stopBits;
    //U8      parityBits;
}tcp232_uart_t;



int tcp232_init(tcp232_rx_t fn);

int tcp232_reset(void);
    
int tcp232_write(U8 *data, U32 len);

int tcp232_set_uart(tcp232_uart_t *set);

int tcp232_get_uart(tcp232_uart_t *set);

int tcp232_set_net(tcp232_net_t *set);

int tcp232_get_net(tcp232_net_t *set);

#endif

