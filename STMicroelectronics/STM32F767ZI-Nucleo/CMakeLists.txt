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

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the toolchain if not defined
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-gcc-cortex-m7.cmake")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(utilities)

# Define the Project
project(stm32f767_threadx C CXX ASM)

# Select the active demo to build (threadx_basic)
set(ACTIVE_DEMO "threadx_basic" CACHE STRING "Active demo name to build: threadx_basic")

# Set up standard paths for find modules
set(STM32_FAMILY "F7")
set(STM32Cube_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/stm32cubef7")

# Find CMSIS and HAL driver packages
find_package(CMSIS REQUIRED)
find_package(STM32HAL REQUIRED COMPONENTS cortex pwr rcc gpio uart dma eth i2c)

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

# Dynamic Middleware Auto-Detection
# Check if the active demo uses NetX Duo by looking for nx_user.h
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/app/demos/${ACTIVE_DEMO}/nx_user.h")
    set(USE_NETXDUO ON)
    set(NX_USER_FILE "${CMAKE_CURRENT_LIST_DIR}/app/demos/${ACTIVE_DEMO}/nx_user.h" CACHE STRING "Enable NetX Duo user configuration" FORCE)
else()
    set(USE_NETXDUO OFF)
endif()

# Check if the active demo has custom tx_user.h; otherwise fallback to app/tx_user.h
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/app/demos/${ACTIVE_DEMO}/tx_user.h")
    set(TX_USER_FILE "${CMAKE_CURRENT_LIST_DIR}/app/demos/${ACTIVE_DEMO}/tx_user.h" CACHE STRING "Enable TX user configuration" FORCE)
    set(TX_USER_FILE_DIR "${CMAKE_CURRENT_LIST_DIR}/app/demos/${ACTIVE_DEMO}")
else()
    set(TX_USER_FILE "${CMAKE_CURRENT_LIST_DIR}/lib/threadx/tx_user.h" CACHE STRING "Enable TX user configuration" FORCE)
    set(TX_USER_FILE_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/threadx")
endif()

# Compile ThreadX Kernel from root shared libs submodule
set(THREADX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../libs/threadx")
add_subdirectory(${THREADX_DIR} threadx)

# Compile FileX Filesystem from root shared libs submodule
set(FILEX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../libs/filex")
add_subdirectory(${FILEX_DIR} filex)

if(USE_NETXDUO)
    # Compile NetX Duo TCP/IP Stack from root shared libs submodule
    set(NXD_ENABLE_FILE_SERVERS OFF CACHE BOOL "Disable FileX dependency in NetX Duo" FORCE)
    set(NETXDUO_DIR "${CMAKE_CURRENT_LIST_DIR}/../../libs/netxduo")
    add_subdirectory(${NETXDUO_DIR} netxduo)
endif()

# 1. Define the Board BSP static library
add_library(board_bsp OBJECT
    app/startup/startup_stm32f767zitx.s
    app/startup/system_stm32f7xx.c
    app/startup/tx_initialize_low_level.S
    app/board_init.c
    app/console.c
    app/stm32f7xx_hal_msp.c
    app/sysmem.c
    app/syscalls.c
)

# Include paths for BSP
target_include_directories(board_bsp PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/app
    ${CMSIS_INCLUDE_DIRS}
    ${STM32HAL_INCLUDE_DIR}
    ${TX_USER_FILE_DIR}
)

# Compile definitions for BSP
target_compile_definitions(board_bsp PRIVATE
    STM32F767xx
    USE_HAL_DRIVER
    STM32F7
)

# Link libraries for BSP (HAL and CMSIS)
target_link_libraries(board_bsp PUBLIC
    stm32cubef7
    threadx
)


# 2. Define conditional NetX Duo driver library target
if(USE_NETXDUO)
    add_library(netx_stm32_driver OBJECT
        lib/netxduo/nx_stm32_eth_driver.c
        lib/netxduo/nx_stm32_phy_driver.c
        lib/netxduo/lan8742.c
    )
    
    # NetX driver needs its local files, HAL, CMSIS, ThreadX, NetX Duo and the active demo folder (for nx_stm32_eth_config.h and nx_user.h)
    target_include_directories(netx_stm32_driver PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/lib/netxduo
        ${CMAKE_CURRENT_LIST_DIR}/app/demos/${ACTIVE_DEMO}
        ${CMAKE_CURRENT_LIST_DIR}/app
        ${CMSIS_INCLUDE_DIRS}
        ${STM32HAL_INCLUDE_DIR}
    )
    
    target_compile_definitions(netx_stm32_driver PRIVATE
        STM32F767xx
        USE_HAL_DRIVER
        STM32F7
    )
    
    target_link_libraries(netx_stm32_driver PUBLIC
        netxduo
        stm32cubef7
    )
endif()

# 3. Add the active demo subdirectory to build the executable target
add_subdirectory(app/demos/${ACTIVE_DEMO})
