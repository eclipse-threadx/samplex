#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT
# 
#  Contributors: 
#     Ali Eissa - 2026 version.

set(DEMO_TARGET "demo_threadx_basic")
add_executable(${DEMO_TARGET}
    main.c
)
set_target_properties(${DEMO_TARGET} PROPERTIES OUTPUT_NAME "mimxrt1064_threadx")

# Include paths for the executable target
target_include_directories(${DEMO_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(${DEMO_TARGET}
    PRIVATE
        board_bsp
        threadx
        mcux_sdk
)

# Apply GCC linker script and print memory usage (utilities.cmake function)
set_target_linker(${DEMO_TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/../../startup/MIMXRT1064xxxxx_flexspi_nor.ld")

# Post-build commands to generate raw .bin and .hex files
post_build(${DEMO_TARGET})
