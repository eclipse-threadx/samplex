/* 
 * Copyright (c) 2026 STMicroelectronics
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

#ifndef _BOARD_INIT_H
#define _BOARD_INIT_H

/* Official STM32CubeMX generated header containing hardware pin defines */
#include "main.h"

/* Nucleo-144 User Button (active high) */
#define BUTTON_PIN USER_Btn_Pin
#define BUTTON_IS_PRESSED ((USER_Btn_GPIO_Port->IDR & USER_Btn_Pin) != 0)

/* Nucleo-144 User LEDs: PB0 (Green), PB7 (Blue), PB14 (Red) mapped via STM32CubeMX defines */
#define LED1_ON()  LD1_GPIO_Port->BSRR = LD1_Pin
#define LED1_OFF() LD1_GPIO_Port->BSRR = (uint32_t)LD1_Pin << 16

#define LED2_ON()  LD2_GPIO_Port->BSRR = LD2_Pin
#define LED2_OFF() LD2_GPIO_Port->BSRR = (uint32_t)LD2_Pin << 16

#define LED3_ON()  LD3_GPIO_Port->BSRR = LD3_Pin
#define LED3_OFF() LD3_GPIO_Port->BSRR = (uint32_t)LD3_Pin << 16

/* Redirect UartHandle directly to STM32CubeMX global huart3 */
extern UART_HandleTypeDef huart3;
#define UartHandle huart3

/* Define prototypes. */
void board_init(void);

#endif // _BOARD_INIT_H
