#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT

if(NOT APP_CONFIG)
    set(APP_CONFIG "starter")
endif()

message(STATUS "Building application with configuration: ${APP_CONFIG}")

set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
set(CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${APP_CONFIG})

set(SOURCES
    ${COMMON_DIR}/startup/system_stm32f4xx.c
    ${COMMON_DIR}/startup/startup_stm32f401xe.s
    ${COMMON_DIR}/startup/tx_initialize_low_level.S

    ${COMMON_DIR}/stm32cubef4/stm32f4xx_hal_msp.c

    ${COMMON_DIR}/board_init.c
    ${COMMON_DIR}/console.c
    ${COMMON_DIR}/newlib_syscalls.c

    ${CONFIG_DIR}/main.c
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
        ${CONFIG_DIR}
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
