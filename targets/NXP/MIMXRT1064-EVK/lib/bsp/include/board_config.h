/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 *
 * Contributors:
 *    Ali Eissa - 2026 version.
 */

// Portions of this file were generated with AI assistance.

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>
#include <stddef.h>

#define BSP_BOARD_NAME          "MIMXRT1064-EVK"
#define BSP_CORE_CLOCK_HZ       600000000UL
#define BSP_CPU_CLOCK_HZ        BSP_CORE_CLOCK_HZ
#define BSP_SYSTEM_CLOCK_HZ     BSP_CORE_CLOCK_HZ
#define BSP_UART_BAUDRATE       115200U

#define BSP_HAS_LED             1
#define BSP_HAS_CONSOLE         1

/* Memory configuration (DTCM data RAM) */
#define BSP_RAM_START           0x20000000UL
#define BSP_RAM_SIZE            0x00020000UL  /* 128 KB DTCM */
#define BSP_RAM_END             (BSP_RAM_START + BSP_RAM_SIZE)
#define BSP_MAIN_STACK_RESERVE  0x00000400UL  /* 1 KB main stack */

extern uint32_t SystemCoreClock;

#endif /* BOARD_CONFIG_H */
