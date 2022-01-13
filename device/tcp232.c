#include "tcp232.h"
#include "drv/uart.h"
#include "drv/gpio.h"
#include "drv/clk.h"
#include "task.h"
#include "myCfg.h"

#define TCP232_BUF_LEN          1000
#define TCP232_PORT             UART_1
#define TCP232_BAUDRATE         115200


#define WAIT_ACK_TIME           500
#define WAIT_QUERY_TIME         500


static handle_t urtHandle=NULL;
static handle_t cfgHandle=NULL;
static handle_t rstHandle=NULL;

static U8 in_at_mode=0;
static U8 tcp232_buf[TCP232_BUF_LEN];



static char* AT_CMD[AT_MAX]={
    "AT+E",
    "AT+Z",
    "AT+VER",
    "AT+ENTM",
    "AT+RELD",
    "AT+MAC",
    "AT+WEBU",
    "AT+WANN",
    "AT+DNS",
    "AT+WEBPORT",
    "AT+UART",
    "AT+SOCK",
    "AT+TCPSE",
    "AT+SOCKLK",
    "AT+SOCKPORT",
    "AT+RFCEN",
    "AT+PDTIME",
    
    "AT+REGEN",
    "AT+REGTCP",
    "AT+REGCLOUD",
    "AT+REGUSR",
    
    "AT+HTPTP",
    "AT+HTPURL",
    "AT+HTPHEAD",
    "AT+HTPCHD",
          
    "AT+HEARTEN",
    "AT+HEARTTP",
    "AT+HEARTTM",
    "AT+HEARTDT",
    
    "AT+SCSLINK",
    "AT+CLIENTRST",
    "AT+INDEXEN",
    "AT+SOCKSL",
    "AT+SHORTO",
    "AT+UARTCLBUF",
    "AT+RSTIM",
    "AT+MAXSK",
    "AT+MID",
    "AT+H",
    
    "AT+CFGTF",
    "AT+PING",
};


#define ATBUF_LEN   200
static U8 atBuf[ATBUF_LEN];
static U16 tcp232_rx_len=0;
static tcp232_rx_t tcp232_rx_fn=NULL;
static void tcp232_uart_rx(U8 *data, U16 data_len)
{
    if(tcp232_rx_fn) {
        tcp232_rx_fn(data, data_len);
    }
    
    tcp232_rx_len = data_len;
    if(in_at_mode && tcp232_rx_len<=ATBUF_LEN) {
        memcpy(atBuf, data, tcp232_rx_len);
    }
}


static int wait_at_ack(char *str, int ms)
{
    U8  len=strlen(str);
    U32 t1=clk2_get_tick();
    
    while(1) {
        if(tcp232_rx_len>0) {
            if(memcmp(atBuf, str, len)==0) {
                return 1;
            }
        }
        
        if(ms>0 && (clk2_get_tick()-t1)>=ms) {
            break;
        }
        
    }
    
    return 0;
}
static int wait_at_query(char *cmd, int ms)
{
    enum{LR=0,LF};
    int idx=0,cnt[2]={0},finish=0;
    U32 t1=clk2_get_tick();
    
    while(1) {
        if(tcp232_rx_len>0) {
            if(cnt[LR]>=2 && cnt[LF]>=2) {
                finish=1;
            }
            
            if(idx<tcp232_rx_len) {
                if(atBuf[idx]=='\r') {
                    cnt[LR]++;
                }
                else if(atBuf[idx]=='\n') {
                    cnt[LF]++;
                }
            }
            
            idx++;
        }
        
        if(ms>0 && (clk2_get_tick()-t1)>=ms) {
            break;
        }
    }
    
    if(finish>0 && !strstr((char*)atBuf, cmd)) {
        return 0;
    }
    
    return -1;
}
static int send_at_cmd(char *cmd)
{
    tcp232_rx_len = 0;
    uart_write(urtHandle, (U8*)cmd, strlen(cmd));
    //delay_ms(500);
    return 0;
}

static int enter_at_mode(void)
{
    int r=0;
    
    in_at_mode = 1;
    send_at_cmd("+++");
    r = wait_at_ack("a", WAIT_ACK_TIME);
    if(r==0) {
        in_at_mode = 0;
        return -1;
    }
    
    send_at_cmd("a");
    r = wait_at_ack("+ok", WAIT_ACK_TIME);
    if(r==0) {
        in_at_mode = 0;
        return -1;
    }
    
    return 0;
}
static int exit_at_mode(void)
{
    int r;
    char cmd[20];
    
    sprintf(cmd, "%s\r", AT_CMD[AT_ENTM]);
    send_at_cmd(cmd);
    r = wait_at_ack("\r\n+OK\r\n", WAIT_ACK_TIME);
    if(r==0) {
        return -1;
    }
    in_at_mode = 0;
    
    return 0;
}

static void uart_cfg_en(U8 en)
{
    U8 hl=en?0:1;
    
    gpio_set_hl(cfgHandle, hl);
}


static void tcp232_io_init(void)
{
    gpio_cfg_t cfg;
    gpio_pin_t rstPin=TCP232_RST_PIN;
    gpio_pin_t cfgPin=TCP232_CFG_PIN;
    
    cfg.mode = MODE_OUTPUT;
    
    cfg.pin = rstPin;
    rstHandle = gpio_init(&cfg);
    gpio_set_hl(rstHandle, 1);
    
    cfg.pin = cfgPin;
    cfgHandle = gpio_init(&cfg);
    gpio_set_hl(cfgHandle, 1);
}


static void tcp232_port_init(tcp232_rx_t fn)
{
    uart_cfg_t cfg;
    
    cfg.baudrate = TCP232_BAUDRATE;
    cfg.mode = MODE_DMA;
    cfg.para.buf = tcp232_buf;
    cfg.para.blen = sizeof(tcp232_buf);
    cfg.para.dlen = 0;
    cfg.para.rx = tcp232_uart_rx;
    tcp232_rx_fn = fn;
    
    cfg.port = TCP232_PORT;
    cfg.useDMA = 0;
    urtHandle = uart_init(&cfg);
}


static void tcp232_test(void)
{
    char tmp[30];
    
    //sprintf(tmp, "%s\r", AT_CMD[AT_VER]);
    sprintf(tmp, "%s", "+++");
    
    //enter_at_mode();
    while(1) {
        send_at_cmd(tmp);
        delay_ms(500);
    }
    //exit_at_mode();
}

int tcp232_init(tcp232_rx_t fn)
{
    tcp232_io_init();
    tcp232_port_init(fn);
    tcp232_reset();
    
    return 0;
}


int tcp232_reset(void)
{
    gpio_set_hl(rstHandle, 0);
    delay_ms(200);
    gpio_set_hl(rstHandle, 1);
    delay_ms(1000);
    
    return 0;
}



int tcp232_set_net(tcp232_net_t *set)
{
    int r;
    char cmd[80];
    
    enter_at_mode();
    
    sprintf(cmd, "%s=%s,%s,%s,%s\r", AT_CMD[AT_WANN], set->mode, set->cliAddr.ip, set->mask, set->gateway);
    send_at_cmd(cmd);
    r = wait_at_ack("\r\n+OK\r\n", WAIT_ACK_TIME);
    if(!r) {
        goto fail;
    }
    
    sprintf(cmd, "%s=TCPC,%s,%d\r", AT_CMD[AT_SOCK], set->svrAddr.ip, set->svrAddr.port);
    send_at_cmd(cmd);
    r = wait_at_ack("\r\n+OK\r\n", WAIT_ACK_TIME);
    if(!r) {
        goto fail;
    }
    
    sprintf(cmd, "%s=%d\r", AT_CMD[AT_SOCKPORT], set->cliAddr.port);
    send_at_cmd(cmd);
    r = wait_at_ack("\r\n+OK\r\n", WAIT_ACK_TIME);
    if(!r) {
        goto fail;
    }
    
    sprintf(cmd, "%s=%s\r", AT_CMD[AT_DNS], set->dns);
    send_at_cmd(cmd);
    r = wait_at_ack("\r\n+OK\r\n", WAIT_ACK_TIME);
    if(!r) {
        goto fail;
    }
    
    exit_at_mode();
    return 0;
    
fail:
    exit_at_mode();
    return -1;
}

int tcp232_get_net(tcp232_net_t *set)
{
    int r;
    char cmd[80];
    
    enter_at_mode();
    
    sprintf(cmd, "%s\r", AT_CMD[AT_WANN]);
    send_at_cmd(cmd);
    r = wait_at_query(AT_CMD[AT_WANN], 100);
    if(!r) {
        goto fail;
    }
    sscanf((char*)atBuf, "%s,%s,%s, %s", (char*)&set->mode, (char*)&set->cliAddr.ip, (char*)set->mask, (char*)&set->gateway);
    
    sprintf(cmd, "%s\r", AT_CMD[AT_SOCKPORT]);
    send_at_cmd(cmd);
    r = wait_at_query(AT_CMD[AT_SOCKPORT], 100);
    if(!r) {
        goto fail;
    }
    sscanf((char*)atBuf, "%u", (int*)&set->cliAddr.port);
    
    sprintf(cmd, "%s\r", AT_CMD[AT_SOCK]);
    send_at_cmd(cmd);
    r = wait_at_ack("\r\n+OK\r\n", 100);
    if(!r) {
        goto fail;
    }
    sscanf((char*)atBuf, "%s%d", (char*)&set->svrAddr.ip, (int*)&set->svrAddr.port);
    
    sprintf(cmd, "%s\r", AT_CMD[AT_DNS]);
    send_at_cmd(cmd);
    r = wait_at_query(AT_CMD[AT_DNS], 100);
    if(!r) {
        goto fail;
    }
    sscanf((char*)atBuf, "%s", (char*)&set->dns);
    
    exit_at_mode();
    return 0;
    
fail:
    exit_at_mode();
    return -1;
    
}

int tcp232_set_uart(tcp232_uart_t *set)
{
    int r;
    char cmd[80];
    
    enter_at_mode();
    
    sprintf(cmd, "%s=%d,%d,%d,NONE,NFC\r", AT_CMD[AT_UART], set->baudRate, set->dataBits, set->stopBits);
    send_at_cmd(cmd);
    r = wait_at_ack("\r\n+OK\r\n", 100);
    if(!r) {
        goto fail;
    }
    
    exit_at_mode();
    return 0;
    
fail:
    exit_at_mode();
    return -1;
}

int tcp232_get_uart(tcp232_uart_t *set)
{
    int r;
    char cmd[80];
    
    enter_at_mode();
    
    sprintf(cmd, "%s\r", AT_CMD[AT_UART]);
    send_at_cmd(cmd);
    r = wait_at_query(AT_CMD[AT_UART], 100);
    if(!r) {
        goto fail;
    }
    sscanf((char*)atBuf, "%c,%c,%c", (U8*)&set->baudRate, (U8*)&set->dataBits, (U8*)&set->stopBits);
    
    exit_at_mode();
    return 0;
    
fail:
    exit_at_mode();
    return -1;
}


    
int tcp232_write(U8 *data, U32 len)
{
    tcp232_rx_len = 0;
    return uart_write(urtHandle, data, len);
}

