/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/license/mit.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef NUCLEO_BSP_H
#define NUCLEO_BSP_H

#include "stm32f4xx_hal.h"

/* Nucleo-F401RE user LED (LD2) on PA5 — UM1724 user manual. */
#define NUCLEO_LED_PIN       GPIO_PIN_5
#define NUCLEO_LED_GPIO_PORT GPIOA

void nucleo_bsp_init(void);
void nucleo_led_on(void);
void nucleo_led_off(void);
void nucleo_led_toggle(void);

#endif /* NUCLEO_BSP_H */
