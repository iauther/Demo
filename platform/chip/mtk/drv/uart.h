#ifndef __UART_Hx__
#define __UART_Hx__

#include "types.h"

typedef void (*rx_fn_t)(U8 *data, U16 data_len);

typedef struct {
    rx_fn_t             rx;
    U8                  *buf;
    U16                 blen;
    U16                 dlen;       //data length
}uart_para_t;
typedef struct {
    U8                  mode;
    U8                  port;
    U8                  baudrate;
    U8                  useDMA;
    uart_para_t         para;
}uart_cfg_t;



handle_t uart_init(uart_cfg_t *cfg);

int uart_deinit(handle_t *h);

int uart_read(handle_t h, U8 *data, U32 len);

int uart_write(handle_t h, U8 *data, U32 len);

int uart_put_char(handle_t h, char c);

int uart_get_char(handle_t h, char *c, U8 block);

int uart_is_read_busy(handle_t h);

int uart_is_write_busy(handle_t h);

int uart_rw(handle_t h, U8 *data, U32 len, U8 rw);

#endif

