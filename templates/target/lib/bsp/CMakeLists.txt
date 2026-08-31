#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/licenses/MIT.
# 
#  SPDX-License-Identifier: MIT

# TODO: Rename target_bsp library name if desired (e.g., custom_bsp)
add_library(target_bsp STATIC
    src/bsp_board.c
    src/bsp_led.c
    src/bsp_console.c
    src/bsp_memory.c
    src/bsp_selftest.c
)

# Export BSP headers and shared abstract C BSP headers to PUBLIC include paths
target_include_directories(target_bsp PUBLIC
    include
    ${SHARED_BSP_DIR}/include
)

# Link ThreadX and vendor HAL dependencies if required
target_link_libraries(target_bsp PRIVATE
    azrtos::threadx
    # TODO: Add vendor HAL library target here if needed (e.g. stm32cubef4)
)
