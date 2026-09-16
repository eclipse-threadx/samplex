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

# Server Executable Target (mimxrt1064_threadx)
set(SERVER_TARGET "demo_netx_trng_server")
add_executable(${SERVER_TARGET}
    main.c
)
set_target_properties(${SERVER_TARGET} PROPERTIES OUTPUT_NAME "mimxrt1064_threadx")

# Include paths for server
target_include_directories(${SERVER_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries for server
target_link_libraries(${SERVER_TARGET}
    PRIVATE
        board_bsp
        threadx
        netxduo
        netx_imxrt_driver
        mcux_sdk
)

# Apply GCC linker script and post-build outputs for server
set_target_linker(${SERVER_TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/../../startup/MIMXRT1064xxxxx_flexspi_nor.ld")
post_build(${SERVER_TARGET})

# Automated Verification Client Executable Target (mimxrt1064_client)
set(CLIENT_TARGET "demo_netx_trng_client")
add_executable(${CLIENT_TARGET}
    client_main.c
)
set_target_properties(${CLIENT_TARGET} PROPERTIES OUTPUT_NAME "mimxrt1064_client")

# Set compile definitions for client
target_compile_definitions(${CLIENT_TARGET}
    PRIVATE
        NETX_CLIENT_NODE=1
)

# Include paths for client
target_include_directories(${CLIENT_TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries for client
target_link_libraries(${CLIENT_TARGET}
    PRIVATE
        board_bsp
        threadx
        netxduo
        netx_imxrt_driver_client
        mcux_sdk
)

# Apply GCC linker script and post-build outputs for client
set_target_linker(${CLIENT_TARGET} "${CMAKE_CURRENT_SOURCE_DIR}/../../startup/MIMXRT1064xxxxx_flexspi_nor.ld")
post_build(${CLIENT_TARGET})
