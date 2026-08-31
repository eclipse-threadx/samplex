#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/licenses/MIT.
# 
#  SPDX-License-Identifier: MIT

set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)

set(SOURCES
    # TODO: Add MCU system init and assembly startup files (.c / .s / .S)
    # Example:
    # ${COMMON_DIR}/startup/system_mcu.c
    # ${COMMON_DIR}/startup/startup_mcu.s
    # ${COMMON_DIR}/startup/tx_initialize_low_level.S

    # This target's ThreadX application. Applications are target-resident:
    # there is no shared apps/ directory to link one from.
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c

    # Link GCC Newlib syscall stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/bsp/src/newlib_stubs.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        azrtos::threadx
        target_bsp
        # TODO: Add vendor HAL libraries here if needed
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${COMMON_DIR}
)

# TODO: Specify target linker script path
# Example: set_target_linker(${PROJECT_NAME} ${COMMON_DIR}/startup/mcu.ld)
