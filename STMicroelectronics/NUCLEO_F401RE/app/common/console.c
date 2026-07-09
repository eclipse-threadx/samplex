/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "stm32f4xx_hal.h"

#include "board_init.h"

int __io_putchar(int ch);
int __io_getchar(void);
int _read(int file, char* ptr, int len);
int _write(int file, char* ptr, int len);

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&UartHandle, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

int __io_getchar(void)
{
    uint8_t ch;
    HAL_UART_Receive(&UartHandle, &ch, 1, HAL_MAX_DELAY);
    return (int)ch;
}

int _read(int file, char* ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
    {
        ptr[i] = (char)__io_getchar();
    }
    return len;
}

int _write(int file, char* ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
    {
        __io_putchar((int)ptr[i]);
    }
    return len;
}
