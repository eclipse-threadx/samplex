#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/licenses/MIT.
# 
#  SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.15)

# TODO: Replace 'TARGET_BOARD_TEMPLATE' with your target board name (e.g. MY_CUSTOM_BOARD)
project(TARGET_BOARD_TEMPLATE C CXX ASM)

# AGENTS.md requires C99 compatibility.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Define root paths. Only libs/ and bsp/ are shared; the toolchain file and any
# CMake helper modules belong to this target, under cmake/.
get_filename_component(WORKSPACE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(SHARED_LIB_DIR "${WORKSPACE_ROOT}/libs")
set(SHARED_BSP_DIR "${WORKSPACE_ROOT}/bsp")

# TODO: Add this target's cmake/ directory and select its toolchain file, e.g.
#   if(NOT CMAKE_TOOLCHAIN_FILE)
#       set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/<arch>-toolchain.cmake")
#   endif()
#   list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Global compiler flags
add_compile_options(
    -g3
    -ffunction-sections
    -fdata-sections
    -fno-strict-aliasing
    -fno-builtin
    -fno-common
    -Wall
    -Wshadow
    -Wdouble-promotion
    -Werror
    -Wno-unused-parameter
)

# Global preprocessor definitions
add_compile_definitions(
    TX_INCLUDE_USER_DEFINE_FILE
)

# TODO: Add vendor SDK / HAL subdirectories here if needed
# Example: add_subdirectory(lib/vendor_hal)

# Add libraries and application targets
add_subdirectory(lib)
add_subdirectory(app)
