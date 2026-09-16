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

function(post_build TARGET)
    if(CMAKE_C_COMPILER_ID STREQUAL "IAR")
        add_custom_target(${TARGET}_bin ALL 
            DEPENDS ${TARGET}
            COMMAND ${CMAKE_IAR_ELFTOOL} --bin ${TARGET}.elf ${TARGET}.bin)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_custom_target(${TARGET}_bin ALL 
            DEPENDS ${TARGET}
            COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${TARGET}> $<TARGET_FILE_DIR:${TARGET}>/$<TARGET_FILE_BASE_NAME:${TARGET}>.bin
            COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${TARGET}> $<TARGET_FILE_DIR:${TARGET}>/$<TARGET_FILE_BASE_NAME:${TARGET}>.hex)
    else()
        message(FATAL_ERROR "Unknown CMAKE_C_COMPILER_ID ${CMAKE_C_COMPILER_ID}")
    endif()
endfunction()

function(set_target_linker TARGET LINKER_SCRIPT)
    if(CMAKE_C_COMPILER_ID STREQUAL "IAR")
        target_link_options(${TARGET} PRIVATE --config ${LINKER_SCRIPT})
        target_link_options(${TARGET} PRIVATE --map=${TARGET}.map)
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_link_options(${TARGET} PRIVATE -T${LINKER_SCRIPT})
        target_link_options(${TARGET} PRIVATE -Wl,-Map=$<TARGET_FILE_DIR:${TARGET}>/$<TARGET_FILE_BASE_NAME:${TARGET}>.map)
        set_target_properties(${TARGET} PROPERTIES SUFFIX ".elf") 
    else()
        message(FATAL_ERROR "Unknown CMAKE_C_COMPILER_ID ${CMAKE_C_COMPILER_ID}")
    endif()
endfunction()

macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()
