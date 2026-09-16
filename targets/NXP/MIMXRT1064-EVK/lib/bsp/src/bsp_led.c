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
 *    Assisted-by: Google DeepMind Antigravity (Gemini 3.8 Flash)
 */

#include "bsp/led.h"
#include "board_config.h"

#include "board.h"
#include "fsl_gpio.h"

void bsp_led_init(void)
{
#if BSP_HAS_LED
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput,
        1, /* Initial output HIGH (active-low LED D18 is OFF) */
        kGPIO_NoIntmode
    };
    GPIO_PinInit(BOARD_USER_LED_GPIO, BOARD_USER_LED_GPIO_PIN, &led_config);
    bsp_led_off();
#endif
}

void bsp_led_on(void)
{
#if BSP_HAS_LED
    /* Active-low: logic 0 turns the LED ON */
    GPIO_PinWrite(BOARD_USER_LED_GPIO, BOARD_USER_LED_GPIO_PIN, 0U);
#endif
}

void bsp_led_off(void)
{
#if BSP_HAS_LED
    /* Active-low: logic 1 turns the LED OFF */
    GPIO_PinWrite(BOARD_USER_LED_GPIO, BOARD_USER_LED_GPIO_PIN, 1U);
#endif
}

void bsp_led_toggle(void)
{
#if BSP_HAS_LED
    uint32_t current = GPIO_PinRead(BOARD_USER_LED_GPIO, BOARD_USER_LED_GPIO_PIN);
    GPIO_PinWrite(BOARD_USER_LED_GPIO, BOARD_USER_LED_GPIO_PIN, (uint8_t)(current ^ 1U));
#endif
}
