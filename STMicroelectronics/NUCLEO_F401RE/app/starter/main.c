/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "tx_api.h"

#include "board_init.h"
#include "cloud_config.h"
#include "nucleo_bsp.h"

#define THREAD_STACK_SIZE 1024

/* Thread Priorities: Lower value means higher priority */
#define BLINK_THREAD_PRIORITY    4
#define WORKER_THREAD_PRIORITY   6
#define REPORTER_THREAD_PRIORITY 10

/* Thread Control Blocks */
static TX_THREAD blink_thread;
static TX_THREAD worker_thread;
static TX_THREAD reporter_thread;

/* Thread Stacks */
static ULONG blink_thread_stack[THREAD_STACK_SIZE / sizeof(ULONG)];
static ULONG worker_thread_stack[THREAD_STACK_SIZE / sizeof(ULONG)];
static ULONG reporter_thread_stack[THREAD_STACK_SIZE / sizeof(ULONG)];

/* Activity counters */
static ULONG blink_counter = 0;
static ULONG worker_counter = 0;
static ULONG reporter_counter = 0;

/* Thread entries */
static void blink_thread_entry(ULONG parameter)
{
    (void)parameter;

    while (1)
    {
        nucleo_led_toggle();
        blink_counter++;
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2); /* Sleep 500ms */
    }
}

static void worker_thread_entry(ULONG parameter)
{
    (void)parameter;

    while (1)
    {
        /* Simulate simple cooperative workload */
        worker_counter++;
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 5); /* Sleep 200ms */
    }
}

static void reporter_thread_entry(ULONG parameter)
{
    (void)parameter;

    printf("\r\n==========================================\r\n");
    printf("Nucleo F401RE ThreadX Multithreading Demo\r\n");
    printf("==========================================\r\n");

    while (1)
    {
        reporter_counter++;
        
        /* Calculate system uptime in seconds */
        ULONG uptime_seconds = tx_time_get() / TX_TIMER_TICKS_PER_SECOND;

        printf("[Reporter] Uptime: %lu s | Blink Thread: %lu runs | Worker Thread: %lu runs | Reporter: %lu runs\r\n",
               uptime_seconds, blink_counter, worker_counter, reporter_counter);

        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 2); /* Report every 2 seconds */
    }
}

void tx_application_define(void* first_unused_memory)
{
    (void)first_unused_memory;
    UINT status;

    /* Create Blink Thread */
    status = tx_thread_create(
        &blink_thread,
        "blink thread",
        blink_thread_entry,
        0,
        blink_thread_stack,
        sizeof(blink_thread_stack),
        BLINK_THREAD_PRIORITY,
        BLINK_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("Blink thread create failed: 0x%08x\r\n", status);
    }

    /* Create Worker Thread */
    status = tx_thread_create(
        &worker_thread,
        "worker thread",
        worker_thread_entry,
        0,
        worker_thread_stack,
        sizeof(worker_thread_stack),
        WORKER_THREAD_PRIORITY,
        WORKER_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("Worker thread create failed: 0x%08x\r\n", status);
    }

    /* Create Reporter Thread */
    status = tx_thread_create(
        &reporter_thread,
        "reporter thread",
        reporter_thread_entry,
        0,
        reporter_thread_stack,
        sizeof(reporter_thread_stack),
        REPORTER_THREAD_PRIORITY,
        REPORTER_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("Reporter thread create failed: 0x%08x\r\n", status);
    }
}

int main(void)
{
    board_init();
    
    /* Start the ThreadX kernel */
    tx_kernel_enter();

    while (1)
    {
    }
}
