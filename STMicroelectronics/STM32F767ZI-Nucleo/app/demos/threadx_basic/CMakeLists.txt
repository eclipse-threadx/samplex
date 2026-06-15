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

add_executable(${PROJECT_NAME}
    main.c
)

# Set compile definitions for our executable
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        STM32F767xx
        USE_HAL_DRIVER
        STM32F7
)

# Include paths for the executable target
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        board_bsp
        threadx
        stm32cubef7
)

# Apply GCC linker script and print memory usage (utilities.cmake function)
set_target_linker(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/../../startup/STM32F767ZITx_FLASH.ld")

# Post-build commands to generate raw .bin and .hex files
post_build(${PROJECT_NAME})
