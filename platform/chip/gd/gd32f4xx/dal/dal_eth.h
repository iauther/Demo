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

#ifndef __DAL_ETH_Hx__
#define __DAL_ETH_Hx__

#include "cfg.h"

#ifdef USE_ETH
#include "lwip/err.h"
#include "lwip/netif.h"


err_t dal_eth_init(struct netif *netif);

#ifdef OS_KERNEL
void dal_eth_input(void* argument);
#else
void dal_eth_input(struct netif *netif);
#endif
void dal_eth_link_check(struct netif *netif);

u32_t sys_jiffies(void);
u32_t sys_now(void);
#endif

#endif
