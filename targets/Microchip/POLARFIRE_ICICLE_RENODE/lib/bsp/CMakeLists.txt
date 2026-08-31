#
# Copyright (c) 2026 Eclipse ThreadX contributors
#
# This program and the accompanying materials are made available
# under the terms of the MIT license which is available at
# https://opensource.org/licenses/MIT.
#
# SPDX-License-Identifier: MIT
#

# Static library compilation rule for Microchip PolarFire SoC Icicle Kit BSP Target
add_library(polarfire_bsp STATIC
    src/bsp_board.c
    src/bsp_led.c
    src/bsp_console.c
    src/board.c
    src/uart.c
    src/plic.c
    src/hwtimer.c
    src/trap.c
    src/bsp_memory.c
    src/bsp_selftest.c
)

# The ThreadX headers are needed for headers only: hwtimer.c checks the board's
# configured tick rate against TX_TIMER_TICKS_PER_SECOND at compile time. The
# BSP links no ThreadX code, so no dependency on the threadx target is implied.
target_include_directories(polarfire_bsp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SAMPLEX_ROOT_DIR}/bsp/include
    PRIVATE
        ${THREADX_DIR}/common/inc
        ${THREADX_DIR}/ports/risc-v64/gnu/inc
)
