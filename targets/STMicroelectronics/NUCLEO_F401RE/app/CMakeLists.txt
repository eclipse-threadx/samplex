#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT

set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
set(STARTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/starter)

set(SOURCES
    ${COMMON_DIR}/startup/system_stm32f4xx.c
    ${COMMON_DIR}/startup/startup_stm32f401xe.s
    ${COMMON_DIR}/startup/tx_initialize_low_level.S

    ${COMMON_DIR}/stm32cubef4/stm32f4xx_hal_msp.c

    # Newlib syscall overrides. Compiled into the executable rather than into
    # libnucleo_bsp.a so the linker always takes these definitions instead of
    # falling back to the libnosys stubs.
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/bsp/src/newlib_stubs.c
    ${STARTER_DIR}/main.c
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        azrtos::threadx
        stm32cubef4
        nucleo_bsp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${COMMON_DIR}
        ${STARTER_DIR}
)

# Linker script
if(CMAKE_C_COMPILER_ID STREQUAL "IAR")
    message(FATAL_ERROR "IAR is not implemented for this target")
else()
    set_target_linker(
        ${PROJECT_NAME}
        ${COMMON_DIR}/startup/NUCLEO_F401RE.ld
    )
endif()

post_build(${PROJECT_NAME})
