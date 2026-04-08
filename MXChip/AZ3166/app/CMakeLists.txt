#  Copyright (c) Microsoft
#  Copyright (c) 2024 Eclipse Foundation
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

if(NOT APP_CONFIG)
    set(APP_CONFIG "starter")
endif()

message(STATUS "Building application with configuration: ${APP_CONFIG}")

set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/common)
set(CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${APP_CONFIG})

# Common sources for all configurations
set(SOURCES
    ${COMMON_DIR}/startup/system_stm32f4xx.c
    ${COMMON_DIR}/startup/startup_stm32f412rx.s
    ${COMMON_DIR}/startup/tx_initialize_low_level.S
    ${COMMON_DIR}/stm32cubef4/stm32f4xx_hal_msp.c
    ${COMMON_DIR}/console.c
    ${COMMON_DIR}/sntp_client.c
    ${COMMON_DIR}/wwd_networking.c
)

# Configuration specific sources and common files that may be overridden
if(APP_CONFIG STREQUAL "arcade")
    list(APPEND SOURCES
        ${CONFIG_DIR}/board_init.c
        ${CONFIG_DIR}/screen.c
        ${CONFIG_DIR}/space.c
        ${CONFIG_DIR}/menu.c
        ${CONFIG_DIR}/button_handler.c
        ${CONFIG_DIR}/fosgame.c
        ${CONFIG_DIR}/slotmachine.c
        ${CONFIG_DIR}/snake.c
        ${CONFIG_DIR}/main.c
        ${CONFIG_DIR}/cloud_config.h
    )
elseif(APP_CONFIG STREQUAL "telemetry")
    list(APPEND SOURCES
        ${COMMON_DIR}/board_init.c
        ${COMMON_DIR}/screen.c
        ${CONFIG_DIR}/telemetry.c
        ${CONFIG_DIR}/main.c
        ${CONFIG_DIR}/cloud_config.h
    )
elseif(APP_CONFIG STREQUAL "mqtt")
    # MQTT depends on Telemetry
    list(APPEND SOURCES
        ${COMMON_DIR}/board_init.c
        ${COMMON_DIR}/screen.c
        ${CONFIG_DIR}/telemetry.c
        ${CONFIG_DIR}/mqtt_client.c
        ${CONFIG_DIR}/main.c
        ${CONFIG_DIR}/cloud_config.h
    )
else() # starter
    list(APPEND SOURCES
        ${COMMON_DIR}/board_init.c
        ${COMMON_DIR}/screen.c
        ${CONFIG_DIR}/main.c
        ${CONFIG_DIR}/cloud_config.h
    )
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    azrtos::threadx
    azrtos::netxduo

    stm32cubef4
    mxchip_bsp
    app_common
    jsmn
    wiced_sdk
    nanoprintf
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC 
        ${COMMON_DIR}
        ${CONFIG_DIR}
)

if(APP_CONFIG STREQUAL "mqtt")
    # Also include telemetry directory for MQTT
    target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/telemetry)
endif()

# Use absolute path to the linker script to support older CMake versions
if(CMAKE_C_COMPILER_ID STREQUAL "IAR")
    message(FATAL_ERROR, "IAR is not currently implemented for this device")
else()
    set_target_linker(${PROJECT_NAME} ${COMMON_DIR}/startup/MXChip_AZ3166.ld)
endif()

post_build(${PROJECT_NAME})
