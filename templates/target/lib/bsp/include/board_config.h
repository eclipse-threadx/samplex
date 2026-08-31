/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/**
 * @file board_config.h
 * @brief Target Hardware Configuration Template
 * 
 * Target board developers must populate these parameters to match their MCU hardware specs.
 */

/* TODO: Set the physical core CPU clock frequency in Hz */
#define BSP_SYSTEM_CLOCK_HZ     84000000

/* TODO: Set the serial debug console baud rate (e.g. 115200) */
#define BSP_UART_BAUDRATE       115200

/* TODO: Set the end address of physical SRAM (SRAM_BASE + SRAM_SIZE_BYTES)
 * Example for 96KB RAM starting at 0x20000000: 0x20000000 + 0x18000 = 0x20018000
 */
#define BSP_RAM_END             0x20018000

/* TODO: Set the number of bytes this board reserves at the top of RAM, if any.
 * bsp_ram_region() subtracts it so an application's byte pool never overlaps
 * the main stack. Boards that keep their stack below ThreadX's first unused
 * address can leave this at 0 and reserve for the C heap instead. */
#define BSP_MAIN_STACK_RESERVE  4096UL

/* Optional hardware peripheral availability flags */
#define BSP_HAS_LED             1
#define BSP_HAS_CONSOLE         1

#endif /* BOARD_CONFIG_H */
