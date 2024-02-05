/*
 * @Author: jiejie
 * @Github: https://github.com/jiejieTop
 * @Date: 2019-12-10 22:18:32
 * @LastEditTime: 2020-04-27 17:15:58
 * @Description: the code belongs to jiejie, please keep the author information and source code according to the license.
 */
#if (MQTT_LIB==2)
 
#ifndef _PLATFORM_TIMER_H_
#define _PLATFORM_TIMER_H_

#include "FreeRTOS.h"

typedef struct platform_timer {
    uint32_t time;
} platform_timer_t;

void platform_timer_init(platform_timer_t* timer);
void platform_timer_cutdown(platform_timer_t* timer, unsigned int timeout);
char platform_timer_is_expired(platform_timer_t* timer);
int platform_timer_remain(platform_timer_t* timer);
unsigned long platform_timer_now(void);
void platform_timer_usleep(unsigned long usec);

#endif

#endif


