#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT

# ThreadX configuration
set(TX_USER_FILE
    "${CMAKE_CURRENT_LIST_DIR}/threadx/tx_user.h"
    CACHE STRING
    "Enable TX user configuration"
)

# Core libraries
add_subdirectory(${SHARED_LIB_DIR}/threadx threadx)

# STM32 platform libraries
add_subdirectory(stm32cubef4)
add_subdirectory(nucleo_bsp)
