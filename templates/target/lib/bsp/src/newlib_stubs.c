/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 */

// Portions of this file were generated with AI assistance.

#ifdef __GNUC__

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include "board_config.h"
#include "bsp/console.h"

/* Placed by the linker script at the first address above .bss, below the stack. */
extern char _end;

/**
 * @brief Dynamic memory allocation heap growth stub for Newlib standard C library.
 *
 * The heap is bounded by BSP_RAM_END from board_config.h. An allocation that
 * would run past the end of physical RAM, overflow the pointer, or shrink the
 * heap below its base is rejected with (void *)-1 and errno set, which is what
 * newlib expects. Returning an out-of-range pointer instead would hand malloc()
 * memory that does not exist.
 *
 * The increment is ptrdiff_t rather than int so the stub stays correct on
 * 64-bit targets.
 */
void* _sbrk(ptrdiff_t incr)
{
    static char* heap = NULL;
    char* prev_heap;

    if (heap == NULL)
    {
        heap = &_end;
    }

    if (incr > 0)
    {
        if ((uintptr_t)heap + (uintptr_t)incr > (uintptr_t)BSP_RAM_END ||
            (uintptr_t)heap + (uintptr_t)incr < (uintptr_t)heap)
        {
            errno = ENOMEM;
            return (void*)-1;
        }
    }
    else if (incr < 0)
    {
        if ((uintptr_t)heap < (uintptr_t)&_end + (uintptr_t)(-incr))
        {
            errno = EINVAL;
            return (void*)-1;
        }
    }

    prev_heap = heap;
    heap += incr;

    return prev_heap;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat* st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

void _exit(int status)
{
    (void)status;
    while (1)
    {
    }
}

void _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
}

int _getpid(void)
{
    return -1;
}

int _read(int file, char* ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

int _write(int file, char* ptr, int len)
{
    (void)file;
    bsp_console_write(ptr, (size_t)len);
    return len;
}

#endif /* __GNUC__ */
