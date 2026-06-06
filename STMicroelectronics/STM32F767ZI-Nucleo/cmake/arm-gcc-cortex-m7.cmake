#  Copyright (c) Microsoft
#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT
# 
#  Contributors: 
#     Microsoft         - Initial version
#     Frédéric Desbiens - 2024 version.
#     Ali Eissa         - 2026 version.

# Define the CPU architecture for ThreadX
set(THREADX_ARCH "cortex_m7")
set(THREADX_TOOLCHAIN "gnu")

# Cortex-M7 compiler options
set(MCPU_FLAGS "-mthumb -mcpu=cortex-m7")
set(VFP_FLAGS "-mfloat-abi=hard -mfpu=fpv5-d16")

include(${CMAKE_CURRENT_LIST_DIR}/arm-gcc-cortex-toolchain.cmake)
