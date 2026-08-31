/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/license/mit.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NUCLEO_CONSOLE_H
#define NUCLEO_CONSOLE_H

#include <stddef.h>

/**
 * @brief Block until length characters have been read from the console UART.
 *
 * Target-local companion to bsp_console_write(). The generic <bsp/console.h>
 * contract is write-only, so console input stays board specific rather than
 * forcing an unimplemented read into every target's BSP.
 *
 * @param data   Buffer receiving the characters read.
 * @param length Number of characters to read.
 */
void nucleo_console_read(char *data, size_t length);

#endif /* NUCLEO_CONSOLE_H */
