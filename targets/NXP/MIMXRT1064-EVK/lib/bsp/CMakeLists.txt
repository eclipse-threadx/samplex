#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/licenses/MIT.
# 
#  SPDX-License-Identifier: MIT

# Board Support Package implementation for NXP MIMXRT1064-EVK
add_library(mimxrt1064_bsp STATIC
    src/bsp_board.c
    src/bsp_led.c
    src/bsp_console.c
    src/bsp_memory.c
    src/bsp_selftest.c
)

target_include_directories(mimxrt1064_bsp
    PUBLIC
        include
        ${SHARED_BSP_DIR}/include
)

target_link_libraries(mimxrt1064_bsp
    PUBLIC
        mimxrt1064_common
        ${SDK_TARGET}
        threadx
)

# Board startup, low-level ThreadX initialization, syscalls, and hardware TRNG
add_library(board_bsp OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/startup/startup_mimxrt1064.S
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/startup/tx_initialize_low_level.S
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../app/trng.c
)

target_include_directories(board_bsp
    PUBLIC
        include
        ${SHARED_BSP_DIR}/include
)

target_link_libraries(board_bsp
    PUBLIC
        mimxrt1064_common
        mimxrt1064_bsp
        ${SDK_TARGET}
        threadx
)
