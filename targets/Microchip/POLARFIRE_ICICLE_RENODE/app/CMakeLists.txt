#
# Copyright (c) 2026 Eclipse ThreadX contributors
#
# This program and the accompanying materials are made available
# under the terms of the MIT license which is available at
# https://opensource.org/licenses/MIT.
#
# SPDX-License-Identifier: MIT
#

# Glob all common C source files for ThreadX with CONFIGURE_DEPENDS
file(GLOB THREADX_COMMON_SOURCES CONFIGURE_DEPENDS "${THREADX_DIR}/common/src/*.c")

# ThreadX Library Target for RISC-V 64-bit GNU Port
add_library(threadx STATIC
    ${THREADX_COMMON_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/common/startup/tx_initialize_low_level.S
    ${THREADX_DIR}/ports/risc-v64/gnu/src/tx_thread_context_restore.S
    ${THREADX_DIR}/ports/risc-v64/gnu/src/tx_thread_context_save.S
    ${THREADX_DIR}/ports/risc-v64/gnu/src/tx_thread_interrupt_control.S
    ${THREADX_DIR}/ports/risc-v64/gnu/src/tx_thread_schedule.S
    ${THREADX_DIR}/ports/risc-v64/gnu/src/tx_thread_stack_build.S
    ${THREADX_DIR}/ports/risc-v64/gnu/src/tx_thread_system_return.S
    ${THREADX_DIR}/ports/risc-v64/gnu/src/tx_timer_interrupt.S
)

target_include_directories(threadx
    PUBLIC
        ${THREADX_DIR}/common/inc
        ${THREADX_DIR}/ports/risc-v64/gnu/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../lib/bsp/include
)

# -------------------------------------------------------------
# Executables
#
# This target builds two. Both link the same BSP, the same startup assembly
# and the same newlib retargeting; they differ only in which application
# source they compile.
#
#   polarfire_icicle_demo  - the board's own LM75 condition-monitoring demo,
#                            in app/main.c. It models a sensor and drives the
#                            MMUART1 receive interrupt, so it stays here with
#                            the board.
#   polarfire_threadx_demo - the shared portable demo from apps/threadx_demo/,
#                            the same source the NUCLEO-F401RE builds. Nothing
#                            about it is RISC-V specific; building it here is
#                            what turns the framework's portability claim into
#                            something CI verifies on a second architecture.
# -------------------------------------------------------------
function(polarfire_add_app TARGET_NAME APP_SOURCE)
    add_executable(${TARGET_NAME}
        ${APP_SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/common/startup/entry.S
        ${CMAKE_CURRENT_SOURCE_DIR}/common/startup/newlib_stubs.c
    )

    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".elf")

    # board_config.h is on this path for newlib_stubs.c, which bounds _sbrk()
    # against the board's heap reservation. The shared application never
    # includes it - and cannot, since it also has to compile for Cortex-M4.
    target_include_directories(${TARGET_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../lib/bsp/include
            ${SAMPLEX_ROOT_DIR}/bsp/include
    )

    target_link_libraries(${TARGET_NAME}
        PRIVATE
            -Wl,--start-group
            polarfire_bsp
            threadx
            -Wl,--end-group
    )
endfunction()

polarfire_add_app(polarfire_icicle_demo ${CMAKE_CURRENT_SOURCE_DIR}/main.c)
polarfire_add_app(polarfire_threadx_demo ${SAMPLEX_ROOT_DIR}/apps/threadx_demo/main.c)

# Applies to the LM75 demo only: it is the executable with the deliberate
# illegal instruction that exercises trap_handler().
option(ENABLE_FAULT_INJECTION "Enable deliberate synchronous fault injection for trap handler testing" OFF)
if(ENABLE_FAULT_INJECTION)
    target_compile_definitions(polarfire_icicle_demo PRIVATE TEST_FAULT_INJECTION=1)
endif()
