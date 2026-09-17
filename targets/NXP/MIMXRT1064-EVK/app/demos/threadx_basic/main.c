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
 */

// Portions of this file were generated with AI assistance.

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "bsp/board.h"
#include "bsp/led.h"
#include "bsp/console.h"
#include "board_config.h"
#include "tx_api.h"

#define HEARTBEAT_THREAD_STACK_SIZE 2048
#define WORKER_THREAD_STACK_SIZE    2048

static TX_THREAD heartbeat_thread;
static ULONG heartbeat_thread_stack[HEARTBEAT_THREAD_STACK_SIZE / sizeof(ULONG)];

static TX_THREAD worker_thread;
static ULONG worker_thread_stack[WORKER_THREAD_STACK_SIZE / sizeof(ULONG)];

static TX_TIMER app_timer;
static volatile ULONG timer_fire_count = 0;

/* Thread Function Prototypes */
static void heartbeat_thread_entry(ULONG thread_input);
static void worker_thread_entry(ULONG thread_input);
static void app_timer_callback(ULONG timer_input);

int main(void)
{
    /* Initialize hardware via BSP interface */
    bsp_board_init();

    printf("\r\n");
    printf("==================================================\r\n");
    printf(" Eclipse ThreadX RTOS on NXP i.MX RT1064-EVK\r\n");
    printf(" Simulated in Antmicro Renode\r\n");
    printf("==================================================\r\n");
    printf("[System] Core Clock: %lu MHz | Tick Rate: %u Hz\r\n",
           SystemCoreClock / 1000000UL,
           TX_TIMER_TICKS_PER_SECOND);
    printf("[System] Initializing ThreadX kernel...\r\n");

    /* Enter the ThreadX kernel */
    tx_kernel_enter();

    return 0;
}

void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    UINT status;

    /* Create Heartbeat Thread (Priority 15 - lower priority) */
    status = tx_thread_create(&heartbeat_thread,
                              "Heartbeat Thread",
                              heartbeat_thread_entry,
                              0,
                              heartbeat_thread_stack,
                              HEARTBEAT_THREAD_STACK_SIZE,
                              15,
                              15,
                              TX_NO_TIME_SLICE,
                              TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        printf("[ERROR] Failed to create Heartbeat Thread (status: 0x%02X)\r\n", status);
    }

    /* Create Worker Thread (Priority 10 - medium priority) */
    status = tx_thread_create(&worker_thread,
                              "Worker Thread",
                              worker_thread_entry,
                              0,
                              worker_thread_stack,
                              WORKER_THREAD_STACK_SIZE,
                              10,
                              10,
                              TX_NO_TIME_SLICE,
                              TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        printf("[ERROR] Failed to create Worker Thread (status: 0x%02X)\r\n", status);
    }

    /* Create Application Timer (Periodic 200 ms / 20 ticks) */
    status = tx_timer_create(&app_timer,
                             "App Timer",
                             app_timer_callback,
                             0,
                             20,  /* Initial ticks (200 ms) */
                             20,  /* Reschedule ticks (200 ms) */
                             TX_AUTO_ACTIVATE);
    if (status != TX_SUCCESS)
    {
        printf("[ERROR] Failed to create App Timer (status: 0x%02X)\r\n", status);
    }

    printf("[System] ThreadX threads and timer registered successfully.\r\n");
}

static void heartbeat_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    ULONG count = 0;
    uint8_t led_state = 0;

    printf("[Heartbeat Thread] Started.\r\n");

    while (1)
    {
        /* Sleep for 50 ticks (500 ms @ 100 Hz) */
        tx_thread_sleep(50);
        count++;

        /* Toggle User LED (D18) on GPIO1 Pin 9 */
        bsp_led_toggle();
        led_state = !led_state;

        printf("[Heartbeat Thread] Heartbeat #%lu (System Tick: %lu | User LED: %s)\r\n",
               count, tx_time_get(), led_state ? "ON" : "OFF");
    }
}

static void worker_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    ULONG iteration = 0;

    printf("[Worker Thread] Started.\r\n");

    while (1)
    {
        /* Sleep for 100 ticks (1000 ms @ 100 Hz) */
        tx_thread_sleep(100);
        iteration++;

        printf("[Worker Thread] Executing periodic task (iteration #%lu, System Tick: %lu)\r\n",
               iteration, tx_time_get());
    }
}

static void app_timer_callback(ULONG timer_input)
{
    (void)timer_input;
    timer_fire_count++;

    /* Report every 5 fires (1 second) */
    if ((timer_fire_count % 5) == 0)
    {
        printf("[App Timer] Kernel timer callback active (total firings: %lu)\r\n",
               timer_fire_count);
    }
}
