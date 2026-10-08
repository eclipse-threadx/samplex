/***************************************************************************
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * AI Disclosure: Portions of this file were generated with Codex (GPT-6).
 * The AI-generated portions may be considered public domain (CC0-1.0)
 * and not subject to the project's licence. Human review is required.
 *
 * SPDX-License-Identifier: MIT and CC0-1.0
 **************************************************************************/
#include <stdio.h>

#include "tx_api.h"

#include "board_init.h"
#include "cmsis_utils.h"
#include "mqtt_client.h"
#include "screen.h"
#include "sntp_client.h"
#include "telemetry.h"
#include "wwd_networking.h"

#include "cloud_config.h"

#define ECLIPSETX_THREAD_STACK_SIZE 4096
#define ECLIPSETX_THREAD_PRIORITY   4
#define MQTT_APP_THREAD_STACK_SIZE 8192
TX_MUTEX mqtt_i2c_mutex;
TX_EVENT_FLAGS_GROUP mqtt_app_flag;
TX_THREAD telemetry_thread;
TX_THREAD mqtt_thread;
ULONG telemetry_thread_stack[ECLIPSETX_THREAD_STACK_SIZE / sizeof(ULONG)];
ULONG mqtt_thread_stack[MQTT_APP_THREAD_STACK_SIZE / sizeof(ULONG)];

void tx_application_define(void* first_unused_memory)
{
    systick_interval_set(TX_TIMER_TICKS_PER_SECOND);
    /* Both threads need these objects before they can start. */
    UINT status = tx_event_flags_create(&mqtt_app_flag, "MQTT event");
    if (status != TX_SUCCESS)
    {
        printf("ERROR: MQTT event creation failed (0x%02x)\r\n", status);
        return;
    }
    status = tx_mutex_create(&mqtt_i2c_mutex, "MQTT sensor/display I2C", TX_INHERIT);
    if (status != TX_SUCCESS)
    {
        printf("ERROR: I2C mutex creation failed (0x%02x)\r\n", status);
        return;
    }

    // Create Telemetry thread
    status = tx_thread_create(&telemetry_thread,
        "Eclipse ThreadX telemetry Thread",
        telemetry_thread_entry,
        0,
        telemetry_thread_stack,
        ECLIPSETX_THREAD_STACK_SIZE,
        ECLIPSETX_THREAD_PRIORITY,
        ECLIPSETX_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("ERROR: Eclipse ThreadX telemetry thread creation failed\r\n");
    }

    // Create MQTT thread
    status = tx_thread_create(&mqtt_thread,
        "Eclipse ThreadX MQTT Thread",
        mqtt_thread_entry,
        0,
        mqtt_thread_stack,
        MQTT_APP_THREAD_STACK_SIZE,
        ECLIPSETX_THREAD_PRIORITY,
        ECLIPSETX_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("ERROR: Eclipse ThreadX MQTT thread creation failed\r\n");
    }}

int main(void)
{
    // Initialize the board
    board_init();

    // Enter the ThreadX kernel
    tx_kernel_enter();

    return 0;
}
