#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT

set(STM32_FAMILY "F4")
set(STM32Cube_DIR "./")

find_package(CMSIS REQUIRED)
find_package(STM32HAL REQUIRED COMPONENTS
    cortex
    dma
    flash
    gpio
    pwr
    rcc
    tim
    uart
    usart
)

set(TARGET stm32cubef4)

add_library(${TARGET} OBJECT
    ${STM32HAL_SOURCES}
)

target_compile_definitions(${TARGET}
    PUBLIC
        STM32F401xE
        USE_HAL_DRIVER
        STM32F4
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMSIS_INCLUDE_DIRS}
        ${STM32HAL_INCLUDE_DIR}
        config
)
