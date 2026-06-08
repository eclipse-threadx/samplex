/*
 * Copyright (c) 2026 Eclipse Foundation
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "tx_api.h"

#include "board_init.h"
#include "cloud_config.h"
#include "nucleo_bsp.h"

#define DEMO_THREAD_STACK_SIZE 1024
#define DEMO_THREAD_PRIORITY   5

static TX_THREAD demo_thread;
static ULONG demo_thread_stack[DEMO_THREAD_STACK_SIZE / sizeof(ULONG)];

static void demo_thread_entry(ULONG parameter)
{
    (void)parameter;

    printf("Nucleo F401RE ThreadX starter running\r\n");

    while (1)
    {
        nucleo_led_toggle();
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2);
    }
}

void tx_application_define(void* first_unused_memory)
{
    (void)first_unused_memory;

    UINT status = tx_thread_create(
        &demo_thread,
        "demo thread",
        demo_thread_entry,
        0,
        demo_thread_stack,
        sizeof(demo_thread_stack),
        DEMO_THREAD_PRIORITY,
        DEMO_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("Thread create failed: 0x%08x\r\n", status);
    }
}

int main(void)
{
    board_init();
    tx_kernel_enter();

    while (1)
    {
    }
}
