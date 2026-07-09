#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.22)

# Set the toolchain if not defined
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-gcc-cortex-m4.cmake")
endif()

# Export compile commands for IDE indexer/clangd support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(nucleo_f401re C ASM)

enable_language(ASM)

set(CMAKE_C_STANDARD 11)

set(SHARED_LIB_DIR
    ${CMAKE_CURRENT_LIST_DIR}/../../libs
)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/cmake
)

include(utilities)

add_subdirectory(lib)
add_subdirectory(app)
