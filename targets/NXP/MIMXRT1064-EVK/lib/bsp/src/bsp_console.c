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
 *    Assisted-by: Google DeepMind Antigravity (Gemini 3.8 Flash)
 */

#include "bsp/console.h"
#include "board_config.h"

#include "fsl_lpuart.h"
#include "board.h"
#include "tx_api.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#if BSP_HAS_CONSOLE
static bsp_console_rx_fn volatile console_rx_handler = NULL;
static void *volatile console_rx_context = NULL;
static TX_MUTEX s_console_mutex;
static volatile int s_console_mutex_created = 0;

static void console_putc(char c)
{
    if (c == '\n')
    {
        while (!(LPUART_GetStatusFlags(LPUART1) & (uint32_t)kLPUART_TxDataRegEmptyFlag))
        {
        }
        LPUART_WriteByte(LPUART1, (uint8_t)'\r');
    }

    while (!(LPUART_GetStatusFlags(LPUART1) & (uint32_t)kLPUART_TxDataRegEmptyFlag))
    {
    }
    LPUART_WriteByte(LPUART1, (uint8_t)c);
}
#endif

void bsp_console_init(void)
{
#if BSP_HAS_CONSOLE
    lpuart_config_t config;

    LPUART_GetDefaultConfig(&config);
    config.baudRate_Bps = BSP_UART_BAUDRATE;
    config.enableTx     = true;
    config.enableRx     = true;

    uint32_t uartClkSrcFreq = BOARD_DebugConsoleSrcFreq();
    LPUART_Init(LPUART1, &config, uartClkSrcFreq);

    /* Set stdout and stderr to unbuffered mode so newlib printf never
     * allocates dynamic heap buffers during multi-threaded execution. */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
#endif
}

void bsp_console_write(const char *data, size_t length)
{
#if BSP_HAS_CONSOLE
    if ((data == NULL) || (length == 0U))
    {
        return;
    }

    int locked = 0;
    /* Only acquire mutex if ThreadX is running, in thread mode, and not inside an ISR */
    if ((__get_IPSR() == 0U) && (tx_thread_identify() != TX_NULL))
    {
        if (!s_console_mutex_created)
        {
            TX_INTERRUPT_SAVE_AREA
            TX_DISABLE
            if (!s_console_mutex_created)
            {
                if (tx_mutex_create(&s_console_mutex, "Console Mutex", TX_NO_INHERIT) == TX_SUCCESS)
                {
                    s_console_mutex_created = 1;
                }
            }
            TX_RESTORE
        }

        if (s_console_mutex_created)
        {
            if (tx_mutex_get(&s_console_mutex, TX_WAIT_FOREVER) == TX_SUCCESS)
            {
                locked = 1;
            }
        }
    }

    for (size_t i = 0U; i < length; i++)
    {
        console_putc(data[i]);
    }

    if (locked)
    {
        tx_mutex_put(&s_console_mutex);
    }
#else
    (void)data;
    (void)length;
#endif
}

void bsp_console_set_rx_handler(bsp_console_rx_fn handler, void *context)
{
#if BSP_HAS_CONSOLE
    console_rx_context = context;
    console_rx_handler = handler;
#else
    (void)handler;
    (void)context;
#endif
}

/* Backward compatibility wrapper for existing code calling console_write */
void console_write(const char *str)
{
    if (str != NULL)
    {
        size_t len = 0;
        while (str[len] != '\0')
        {
            len++;
        }
        bsp_console_write(str, len);
    }
}

/* C runtime newlib redirection */
int _write(int file, char *ptr, int len)
{
    (void)file;
    if (len > 0 && ptr != NULL)
    {
        bsp_console_write(ptr, (size_t)len);
    }
    return len;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
    {
        while (!(LPUART_GetStatusFlags(LPUART1) & (uint32_t)kLPUART_RxDataRegFullFlag))
        {
        }
        ptr[i] = (char)LPUART_ReadByte(LPUART1);
    }
    return len;
}
