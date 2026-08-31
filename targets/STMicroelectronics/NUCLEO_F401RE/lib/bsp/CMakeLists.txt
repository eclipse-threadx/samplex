#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT

# Board support implementation for the ST NUCLEO-F401RE.
# newlib_stubs.c is deliberately absent: the newlib syscall overrides are
# compiled straight into the application so they are never subject to static
# archive extraction order. See ../../app/CMakeLists.txt.
add_library(nucleo_bsp STATIC
    src/bsp_board.c
    src/bsp_led.c
    src/bsp_console.c
    src/bsp_memory.c
    src/bsp_selftest.c
)

target_include_directories(nucleo_bsp
    PUBLIC
        include
        ${SAMPLEX_ROOT_DIR}/bsp/include
)

target_link_libraries(nucleo_bsp
    PUBLIC
        stm32cubef4
)
