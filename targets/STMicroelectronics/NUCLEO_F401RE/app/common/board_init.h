/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARD_INIT_H
#define BOARD_INIT_H

#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef UartHandle;

void board_init(void);

#endif /* BOARD_INIT_H */
