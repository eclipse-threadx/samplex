/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 *
 * Contributors:
 *    Ali Eissa - 2026 version.
 */

// Portions of this file were generated with AI assistance.

#include "bsp/memory.h"
#include "board_config.h"
#include <stdint.h>
#include <stddef.h>

void bsp_ram_region(void *first_unused, void **base, size_t *size)
{
    const uintptr_t start = (uintptr_t)first_unused;
    const uintptr_t end   = (uintptr_t)BSP_RAM_END - (uintptr_t)BSP_MAIN_STACK_RESERVE;

    *base = first_unused;
    *size = (end > start) ? (size_t)(end - start) : (size_t)0U;
}
