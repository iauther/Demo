/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * File Name          : ethernetif.h
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "lwip/err.h"
#include "lwip/netif.h"


err_t ethernetif_init(struct netif *netif);

#ifdef OS_KERNEL
void ethernetif_input(void* argument);
#else
void ethernetif_input(struct netif *netif);
#endif
void ethernet_link_check(struct netif *netif);

u32_t sys_jiffies(void);
u32_t sys_now(void);

#endif
