/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available 
 * under the terms of the MIT license which is available at
 * https://opensource.org/license/mit.
 *
 * SPDX-License-Identifier: MIT
 *
 * Contributors:
 *    Ali Eissa - 2026 version.
 *    Assisted-by: Google DeepMind Antigravity (Gemini 3.8 Flash)
 */

#include "tx_api.h"
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/reent.h>

/**
 * Pointer to the current high watermark of the heap usage
 */
static uint8_t *__sbrk_heap_end = NULL;

/**
 * Depth and interrupt posture for re-entrant newlib malloc locking
 */
static unsigned int s_malloc_lock_posture = 0;
static uint32_t     s_malloc_lock_depth = 0;

void __malloc_lock(struct _reent *reent)
{
    (void)reent;
    TX_INTERRUPT_SAVE_AREA
    TX_DISABLE
    if (s_malloc_lock_depth == 0U)
    {
        s_malloc_lock_posture = interrupt_save;
    }
    s_malloc_lock_depth++;
}

void __malloc_unlock(struct _reent *reent)
{
    (void)reent;
    if (s_malloc_lock_depth == 0U)
    {
        return;
    }
    s_malloc_lock_depth--;
    if (s_malloc_lock_depth == 0U)
    {
        TX_INTERRUPT_SAVE_AREA
        interrupt_save = s_malloc_lock_posture;
        TX_RESTORE
    }
}

/**
 * @brief _sbrk() allocates memory to the newlib heap and is used by malloc.
 */
void *_sbrk(ptrdiff_t incr)
{
    extern uint8_t _end;
    extern uint8_t __heap_limit;
    const uint8_t *max_heap = &__heap_limit;
    uint8_t *prev_heap_end;

    TX_INTERRUPT_SAVE_AREA
    TX_DISABLE

    /* Initialize heap end at first call */
    if (NULL == __sbrk_heap_end)
    {
        __sbrk_heap_end = &_end;
    }

    /* Protect heap from growing beyond linker-defined heap limit without overflow */
    if (incr > 0)
    {
        if ((uintptr_t)incr > (uintptr_t)(max_heap - __sbrk_heap_end))
        {
            TX_RESTORE
            errno = ENOMEM;
            return (void *)-1;
        }
    }
    else if (incr < 0)
    {
        uintptr_t dec = (uintptr_t)(-incr);
        if (dec > (uintptr_t)(__sbrk_heap_end - &_end))
        {
            TX_RESTORE
            errno = EINVAL;
            return (void *)-1;
        }
    }

    prev_heap_end = __sbrk_heap_end;
    __sbrk_heap_end += incr;

    TX_RESTORE
    return (void *)prev_heap_end;
}
