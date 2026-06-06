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

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_C_STANDARD 99)

# Set the toolchain if not defined
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-gcc-cortex-m7.cmake")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(utilities)

# Define the Project
project(stm32f767_threadx C CXX ASM)

# Define ThreadX User Configurations
set(TX_USER_FILE "${CMAKE_CURRENT_LIST_DIR}/lib/threadx/tx_user.h" CACHE STRING "Enable TX user configuration")
set(TX_USER_FILE_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/threadx")

# Set up standard paths for find modules
set(STM32_FAMILY "F7")
set(STM32Cube_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/stm32cubef7")

# Find CMSIS and HAL driver packages
find_package(CMSIS REQUIRED)
find_package(STM32HAL REQUIRED COMPONENTS cortex pwr rcc gpio uart dma eth)

# Compile the STM32F7xx HAL Driver Library as an Object Library
set(HAL_TARGET stm32cubef7)
add_library(${HAL_TARGET} OBJECT ${STM32HAL_SOURCES})

target_compile_definitions(${HAL_TARGET}
    PUBLIC
        STM32F767xx
        USE_HAL_DRIVER
        STM32F7
)

target_include_directories(${HAL_TARGET}
    PUBLIC
        ${CMSIS_INCLUDE_DIRS}
        ${STM32HAL_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/app
)

# Compile ThreadX Kernel from root shared libs submodule
set(THREADX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../libs/threadx")
add_subdirectory(${THREADX_DIR} threadx)

# Create the Main Executable
set(EXE_TARGET stm32f767_threadx)

add_executable(${EXE_TARGET}
    app/startup/startup_stm32f767zitx.s
    app/startup/system_stm32f7xx.c
    app/startup/tx_initialize_low_level.S
    app/board_init.c
    app/console.c
    app/ethernet_phy.c
    app/main.c
    app/stm32f7xx_hal_msp.c
    app/sysmem.c
    app/syscalls.c
)

# Set compile definitions for our executable
target_compile_definitions(${EXE_TARGET}
    PRIVATE
        STM32F767xx
        USE_HAL_DRIVER
        STM32F7
)

# Include paths
target_include_directories(${EXE_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/app
        ${CMSIS_INCLUDE_DIRS}
        ${STM32HAL_INCLUDE_DIR}
        ${TX_USER_FILE_DIR}
)

# Link libraries (includes ThreadX kernel and HAL object libraries)
target_link_libraries(${EXE_TARGET}
    PRIVATE
        threadx
        stm32cubef7
)

# Apply GCC linker script and print memory usage (utilities.cmake function)
set_target_linker(${EXE_TARGET} "${CMAKE_CURRENT_LIST_DIR}/app/startup/STM32F767ZITx_FLASH.ld")

# Post-build commands to generate raw .bin and .hex files
post_build(${EXE_TARGET})
