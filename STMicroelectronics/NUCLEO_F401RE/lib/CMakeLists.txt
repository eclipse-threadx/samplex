#  Copyright (c) 2026 Eclipse Foundation
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
# Same nanoprintf layout as MXChip/AZ3166 (deps/lib/nanoprintf).
set(NANOPRINTF_DIR "${CMAKE_SOURCE_DIR}/../../MXChip/AZ3166/deps/lib/nanoprintf")
add_subdirectory(${NANOPRINTF_DIR} nanoprintf)

# STM32 platform libraries
add_subdirectory(stm32cubef4)
add_subdirectory(nucleo_bsp)
