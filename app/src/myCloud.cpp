#include "myCloud.h"
#include "log.h"

#include <alibabacloud/iot_20180120.hpp>
#include <alibabacloud/open_api.hpp>
#include <boost/any.hpp>
#include <darabonba/console.hpp>
#include <darabonba/util.hpp>
#include <iostream>
#include <map>

//#pragma comment(lib, "lib/xxxx.lib")
//https://help.aliyun.com/zh/iot/developer-reference/use-iot-platform-sdk-for-cpp

myCloud::myCloud()
{
    saveFlag = 0;
}

myCloud::~myCloud()
{

}


int myCloud::init(void)
{
	return 0;
}


int myCloud::deinit(void)
{
	return 0;
}


int myCloud::conn(void)
{
	return 0;
}


int myCloud::disconn(void)
{
	return 0;
}


int myCloud::read(void *data, int len)
{
	return 0;
}


int myCloud::write(void *data, int len)
{
	return 0;
}


