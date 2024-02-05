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

#ifndef __AT2_Hx__
#define __AT2_Hx__

#define AT_MAX    5
typedef int (*notify_t)(int dev, int evt, void *data, int len);

typedef struct {
    int (*print)(const char *fmt, ...);
    int (*tick_get)(void);
    int (*delay_ms)(int ms);
    
    //io
    void* (*io_init)(void);
    int (*io_free)(void *h);
    int (*io_read)(void *h, void *data, int len);
    int (*io_write)(void *h, void *data, int len);
    
    //mutex
    void* (*mtx_init)(void);
    int (*mtx_free)(void *h);
    int (*mtx_hold)(void *h);
    int (*mtx_release)(void *h);
    
    int (*tmr_init)();
    int (*tmr_free)(void *h);
    
    int (*task_init)();
    int (*task_free)(void *h);
    
    int (*mem_malloc)(int len);
    int (*mem_free)(void *p);
}at_sys_t;


typedef struct {
    int (*init)();
    int (*deinit)();
    
    void* (*conn)(void *para);
    int (*disconn)(void *conn);
    
    int (*write)(void *conn, void *data, int len, int timeout);
    int (*recv)(void *conn, void *data, int len, int timeout);
}at_dev_t;


int at_init(at_sys_t *sys);

int at_install(int dev, at_dev_t *dev);
int at_unstall(int dev);

void* at_conn(int dev, void *para);
int at_disconn(int dev, void *conn);

int at_send(int dev, void *conn, void *data, int len, int timeout);
int at_set_notify(int dev, notify_t notify);

#endif

