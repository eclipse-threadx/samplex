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

#ifndef _ETHERNET_PHY_H
#define _ETHERNET_PHY_H

#include "stm32f7xx_hal.h"

/* Global Ethernet Handle */
extern ETH_HandleTypeDef heth;
extern ETH_TxPacketConfig TxConfig;

/* Function Prototypes */
void ethernet_phy_init(void);

#endif // _ETHERNET_PHY_H
