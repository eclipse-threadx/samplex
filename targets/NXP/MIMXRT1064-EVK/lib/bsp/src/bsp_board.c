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

#include "bsp/board.h"
#include "bsp/led.h"
#include "bsp/console.h"
#include "board_config.h"

#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_iomuxc.h"
#include "fsl_gpio.h"

void bsp_board_init(void)
{
    /* 1. Configure the Memory Protection Unit if supported by hardware */
    if (((MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos) >= 12)
    {
        BOARD_ConfigMPU();
    }

    /* 2. Configure Pin Muxing (UART1 TX/RX pins) */
    BOARD_InitPins();

    /* 3. Configure User LED Pin Muxing (GPIO_AD_B0_09 -> GPIO1_IO09) */
    CLOCK_EnableClock(kCLOCK_Iomuxc);
    IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_09_GPIO1_IO09, 0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_09_GPIO1_IO09, 0x10B0u);

    /* 4. Configure System Clocks (600 MHz AHB core clock) */
    BOARD_BootClockRUN();

    /* 5. Initialize User LED and Console via standard BSP interfaces */
    bsp_led_init();
    bsp_console_init();

    /* 6. Configure Ethernet Pin Muxing (RMII and MDC/MDIO) */
    BOARD_InitENET();
}
