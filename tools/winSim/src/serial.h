#ifndef __SERIAL_Hx__
#define __SERIAL_Hx__

#include <windows.h>
#include <string> 
#include "types.h"

typedef void(*rxCallback)(int len);


enum BaudRate
{
	BaudRate110 = 110,       ///< 110
	BaudRate300 = 300,       ///< 300
	BaudRate600 = 600,       ///< 600
	BaudRate1200 = 1200,     ///< 1200
	BaudRate2400 = 2400,     ///< 2400
	BaudRate4800 = 4800,     ///< 4800
	BaudRate9600 = 9600,     ///< 9600 recommend 推荐
	BaudRate14400 = 14400,   ///< 14400
	BaudRate19200 = 19200,   ///< 19200
	BaudRate38400 = 38400,   ///< 38400
	BaudRate56000 = 56000,   ///< 56000
	BaudRate57600 = 57600,   ///< 57600
	BaudRate115200 = 115200, ///< 115200
	BaudRate128000 = 128000, ///< 12800
	BaudRate256000 = 256000,

};


enum DataBits
{
	DataBits5 = 5, ///< 5 data bits 5位数据位
	DataBits6 = 6, ///< 6 data bits 6位数据位
	DataBits7 = 7, ///< 7 data bits 7位数据位
	DataBits8 = 8  ///< 8 data bits 8位数据位
};

enum Parity
{
	ParityNone = 0,  ///< No Parity 无校验
	ParityOdd = 1,   ///< Odd Parity 奇校验
	ParityEven = 2,  ///< Even Parity 偶校验
	ParityMark = 3,  ///< Mark Parity 1校验
	ParitySpace = 4, ///< Space Parity 0校验
};

enum StopBits
{
	StopOne = 0,        ///< 1 stop bit 1位停止位
	StopOneAndHalf = 1, ///< 1.5 stop bit 1.5位停止位 - This is only for the Windows platform
	StopTwo = 2         ///< 2 stop bit 2位停止位
};




handle_t serial_init(void);
int serial_free(handle_t *h);

int serial_open(handle_t h, int port, int baudrate, int async);
int serial_close(handle_t h);

int serial_setup(handle_t h, int baudrate, int bytesize, int parity, int stopbits);

int  serial_getLength(handle_t h);
int  serial_read(handle_t h, void* data, int len);
int  serial_write(handle_t h, void* data, int len);
int  serial_is_opened(handle_t h);


#endif
