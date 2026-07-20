/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/license/mit.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARD_INIT_H
#define BOARD_INIT_H

#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef UartHandle;

void board_init(void);

#endif /* BOARD_INIT_H */
