/* 
 * Copyright (c) 2026 Eclipse ThreadX contributors
 * 
 *  This program and the accompanying materials are made available 
 *  under the terms of the MIT license which is available at
 *  https://opensource.org/license/mit.
 * 
 *  SPDX-License-Identifier: MIT
 * 
 *  Contributors:
 *     Ali Eissa - 2026 version.
 */

#ifndef NX_STM32_ETH_CONFIG_H
#define NX_STM32_ETH_CONFIG_H

#include "stm32f7xx_hal.h"
#include "lan8742.h"

extern ETH_HandleTypeDef heth;
#define eth_handle heth

/* Define the PHY physical address */
#define LAN8742_PHY_ADDRESS 0

/* This define enables the call of nx_eth_init() from the interface layer.*/
/* #define NX_DRIVER_ETH_HW_IP_INIT */
void nx_eth_init(void);

#endif /* NX_STM32_ETH_CONFIG_H */
