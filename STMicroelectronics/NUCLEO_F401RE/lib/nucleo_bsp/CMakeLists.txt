#  Copyright (c) 2026 Eclipse ThreadX contributors
# 
#  This program and the accompanying materials are made available 
#  under the terms of the MIT license which is available at
#  https://opensource.org/license/mit.
# 
#  SPDX-License-Identifier: MIT

set(SOURCES
    nucleo_bsp.c
)

set(TARGET nucleo_bsp)

add_library(${TARGET} OBJECT
    ${SOURCES}
)

target_link_libraries(${TARGET}
    PUBLIC
        stm32cubef4
)

target_include_directories(${TARGET}
    PUBLIC
        .
)
