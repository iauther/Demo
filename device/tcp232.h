#ifndef __TCP232_Hx__
#define __TCP232_Hx__

#include "types.h"
#include "drv/uart.h"

typedef void (*tcp232_rx_t)(U8 *data, U16 data_len);

enum {
    AT_E=0,         //��/�رջ��Թ���
    AT_Z,           //����ģ��
    AT_VER,         //��ѯ�汾��
    AT_ENTM,        //�˳� AT ָ��ģʽ
    AT_RELD,        //�ָ���������
    AT_MAC,         //��ѯģ�� MAC
    AT_WEBU,        //����/��ѯ�û���������
    AT_WANN,        //����/��ѯ WAN �ڲ���
    AT_DNS,         //����/��ѯ DNS ��������ַ
    AT_WEBPORT,     //����/��ѯ��ҳ�˿ں�
    AT_UART,        //����/��ѯ���ڲ���
    AT_SOCK,        //����/��ѯ SOCK ����
    AT_TCPSE,       //����/��ѯ�Ƿ��ߵ�������
    AT_SOCKLK,      //��ѯ TCP ����״̬
    AT_SOCKPORT,    //����/��ѯ���ض˿ں�
    AT_RFCEN,       //����/��ѯ�� RFC2217 ʹ��
    AT_PDTIME,      //��ѯ����ʱ��
    
    AT_REGEN,       //����/��ѯע�������
    AT_REGTCP,      //����/��ѯע���ִ�л�
    AT_REGCLOUD,    //����/��ѯ͸�����û���������
    AT_REGUSR,      //����/��ѯ�û��Զ���ע�������
    
    AT_HTPTP,       //����/��ѯ Httpd Client ģʽ�£�HTTP ������ʽ
    AT_HTPURL,      //����/��ѯ Httpd Client ģʽ�µ� URL
    AT_HTPHEAD,     //����/��ѯ Httpd Client ģʽ�°�ͷ
    AT_HTPCHD,      //����/��ѯ HTP ȥ��ͷ����
    
    AT_HEARTEN,     //����/��ѯ������ʹ��
    AT_HEARTTP,     //����/��ѯ���������ͷ�ʽ
    AT_HEARTTM,     //����/��ѯ������ʱ��
    AT_HEARTDT,     //����/��ѯ�Զ�������������

    AT_SCSLINK,     //����/��ѯ Socket ����״ָ̬ʾ����
    AT_CLIENTRST,   //����/��ѯ TCP Client ģʽ���Ӷ��ʧ�� Reset ����
    AT_INDEXEN,     //����/��ѯ index ����
    AT_SOCKSL,      //����/��ѯ�����ӹ���
    AT_SHORTO,      //����/��ѯ������ʱ��
    AT_UARTCLBUF,   //����/��ѯģ������ǰ�Ƿ������ڻ���
    AT_RSTIM,       //����/��ѯ��ʱ����ʱ��
    AT_MAXSK,       //����/��ѯ TCP Server ���� Client ���ֵ
    AT_MID,         //����/��ѯģ������
    AT_H,           //����

    AT_CFGTF,       //��ǰ��������Ϊ�û�Ĭ�ϲ���
    AT_PING,        //���� ping �� 
    
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

