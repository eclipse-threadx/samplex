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
/* MISRA C:2012/2023 Rule 15.5 deviation: early returns keep startup cleanup
 * adjacent to failure checks. Rule 17.7 deviation: cleanup calls on valid
 * ThreadX/HAL objects deliberately discard their return status. */
#include "cloud_config.h"
#include "mqtt_client.h"
#include "nxd_uprotocol.h"
#include "powertrain_display.h"
#include "sntp_client.h"
#include "stm32f4xx_hal.h"
#include "telemetry.h"
#include "uprotocol_config.h"
#include "wwd_networking.h"

#include <stdio.h>
#include <string.h>

#define STRLEN(s) (sizeof(s) - 1u)
#define UP_WAIT (5u * TX_TIMER_TICKS_PER_SECOND)
#define UP_POLL ((TX_TIMER_TICKS_PER_SECOND + 9u) / 10u)

/* Large control blocks and buffers stay off the application thread stack. */
static ULONG mqtt_client_stack[MQTT_CLIENT_STACK_SIZE / sizeof(ULONG)];
static NXD_MQTT_CLIENT mqtt_client;
static NXD_UPROTOCOL_CLIENT transport;
static NXD_UPROTOCOL_CONFIG configuration;
static NXD_UPROTOCOL_MESSAGE outgoing;
static NXD_UPROTOCOL_URI telemetry_uri;
static NXD_UPROTOCOL_URI powertrain_uri;
static UCHAR receive_buffer[UPROTOCOL_RECEIVE_SIZE];
static CHAR telemetry_buffer[TELEMETRY_BUFFER_SIZE];
static RNG_HandleTypeDef uuid_rng;

/* The network startup has synchronized this clock before transport creation. */
static uint64_t epoch_milliseconds(VOID *context)
{
    NX_PARAMETER_NOT_USED(context);
    return sntp_time_get_ms();
}

/* Generate fresh UUID entropy; never substitute predictable bytes on failure.
 * This handle is used only by the MQTT application thread. */
static UINT uuid_random(UCHAR bytes[10])
{
    UINT i;
    uint32_t word = 0;
    for (i = 0; i < 10u; i++)
    {
        if ((i % 4u) == 0u && HAL_RNG_GenerateRandomNumber(&uuid_rng, &word) != HAL_OK)
        {
            return NXD_UPROTOCOL_INTERNAL;
        }
        bytes[i] = (UCHAR)(word >> (8u * (i % 4u)));
    }
    return NXD_UPROTOCOL_OK;
}

/* Listener payload is a bounded borrowed byte span, not a C string. */
static VOID powertrain_received(const NXD_UPROTOCOL_MESSAGE *message, VOID *context)
{
    bool displayed;
    NX_PARAMETER_NOT_USED(context);
    if (message->type != NXD_UPROTOCOL_PUBLISH ||
        message->payload_format != NXD_UPROTOCOL_PAYLOAD_JSON)
    {
        printf("Ignoring powertrain message without JSON publish attributes\r\n");
        return;
    }
    if (tx_mutex_get(&mqtt_i2c_mutex, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
        return;
    }
    displayed = powertrain_display_update((const char *)message->payload, message->payload_length);
    tx_mutex_put(&mqtt_i2c_mutex);
    if (!displayed)
    {
        printf("Ignoring powertrain JSON without a valid Mode string\r\n");
    }
}

/* JSON content stays unchanged; uProtocol supplies identity, UUID and TTL. */
static UINT send_telemetry(VOID)
{
    UCHAR entropy[10];
    UINT status = uuid_random(entropy);
    if (status != NXD_UPROTOCOL_OK)
    {
        return status;
    }
    memset(&outgoing, 0, sizeof(outgoing));
    memset(telemetry_buffer, 0, sizeof(telemetry_buffer));
    get_current_telemetry_string(telemetry_buffer);
    status = nxd_uprotocol_uuid_create(&outgoing.id, epoch_milliseconds(NX_NULL), entropy);
    if (status != NXD_UPROTOCOL_OK)
    {
        return status;
    }
    outgoing.type = NXD_UPROTOCOL_PUBLISH;
    outgoing.source = telemetry_uri;
    outgoing.priority = NXD_UPROTOCOL_CS1;
    outgoing.present = NXD_UPROTOCOL_HAS_TTL;
    outgoing.ttl = UPROTOCOL_TELEMETRY_TTL_MS;
    outgoing.payload_format = NXD_UPROTOCOL_PAYLOAD_JSON;
    outgoing.payload = (const UCHAR *)telemetry_buffer;
    outgoing.payload_length = (UINT)strlen(telemetry_buffer);
    return nxd_uprotocol_send(&transport, &outgoing, UP_WAIT);
}

/* Polling services both incoming publications and the addon's reconnect timer.
 * Each pass drains a bounded batch so incoming traffic cannot starve telemetry. */
static VOID transport_loop(VOID)
{
    UINT registered = 0;
    UINT pending = 0;
    UINT previous_status = NXD_UPROTOCOL_OK;
    UINT status;
    UINT i;
    ULONG events;
    for (;;)
    {
        events = 0;
        tx_event_flags_get(&mqtt_app_flag, MQTT_MESSAGE_READY, TX_OR_CLEAR, &events, UP_POLL);
        if (events & MQTT_MESSAGE_READY)
        {
            pending = 1;
        }
        for (i = 0; i < 4u; i++)
        {
            status = nxd_uprotocol_process(&transport, receive_buffer, sizeof(receive_buffer), UP_WAIT);
            if (status == NXD_UPROTOCOL_NOT_FOUND)
            {
                status = NXD_UPROTOCOL_OK;
                break;
            }
            if (status != NXD_UPROTOCOL_OK)
            {
                break;
            }
        }
        if (status != NXD_UPROTOCOL_OK && status != previous_status)
        {
            printf("uProtocol process: UCode=%u MQTT=0x%x\r\n", status, transport.last_mqtt_status);
        }
        previous_status = status;
        if (!transport.connected)
        {
            continue;
        }
        if (!registered)
        {
            status = nxd_uprotocol_register_listener(&transport, &powertrain_uri, NX_NULL,
                                                     powertrain_received, NX_NULL, UP_WAIT);
            if (status != NXD_UPROTOCOL_OK)
            {
                printf("uProtocol listener registration failed: %u\r\n", status);
                tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
                continue;
            }
            registered = 1;
            printf("Listening for %s\r\n", UPROTOCOL_POWERTRAIN_URI);
        }
        if (pending)
        {
            /* A failed send may have reached the broker; publish fresh sensor
             * data with a fresh UUID on the next telemetry event. */
            pending = 0;
            status = send_telemetry();
            printf("uProtocol telemetry: UCode=%u\r\n", status);
        }
    }
}

/* Attach the transport to an otherwise unused MQTT client. The addon owns its
 * acknowledgement/disconnect callbacks; do not install raw MQTT callbacks. */
static VOID mqtt_thread_work(NX_IP *ip, NX_PACKET_POOL *pool)
{
    UINT status;
    memset(&configuration, 0, sizeof(configuration));
    if (nxd_uprotocol_uri_parse(UPROTOCOL_LOCAL_URI, &configuration.local_uri) ||
        nxd_uprotocol_uri_parse(UPROTOCOL_TELEMETRY_URI, &telemetry_uri) ||
        nxd_uprotocol_uri_parse(UPROTOCOL_POWERTRAIN_URI, &powertrain_uri))
    {
        printf("Invalid uProtocol URI configuration\r\n");
        return;
    }
    configuration.mode = NXD_UPROTOCOL_E2E;
    configuration.server.nxd_ip_version = NX_IP_VERSION_V4;
    configuration.server.nxd_ip_address.v4 = MQTT_LOCAL_BROKER_IP;
    configuration.port = MQTT_BROKER_PORT;
    configuration.keepalive = MQTT_KEEP_ALIVE_TIMER;
    configuration.clean_start = 1;
    configuration.session_expiry = 0;
    configuration.maximum_packet_size = UPROTOCOL_MAXIMUM_PACKET_SIZE;
    configuration.clock = epoch_milliseconds;

    __HAL_RCC_RNG_CLK_ENABLE();
    uuid_rng.Instance = RNG;
    if (HAL_RNG_Init(&uuid_rng) != HAL_OK)
    {
        printf("UUID RNG initialization failed\r\n");
        return;
    }
    status = nxd_mqtt_client_create(&mqtt_client, MQTT_CLIENT_NAME, MQTT_CLIENT_NAME,
                                    STRLEN(MQTT_CLIENT_NAME), ip, pool,
                                    mqtt_client_stack, sizeof(mqtt_client_stack),
                                    MQTT_THREAD_PRIORTY, NX_NULL, 0);
    if (status != NX_SUCCESS)
    {
        printf("MQTT create failed: 0x%x\r\n", status);
        HAL_RNG_DeInit(&uuid_rng);
        return;
    }
    status = nxd_uprotocol_create(&transport, &mqtt_client, &configuration);
    if (status != NXD_UPROTOCOL_OK)
    {
        printf("uProtocol create failed: %u\r\n", status);
        nxd_mqtt_client_delete(&mqtt_client);
        HAL_RNG_DeInit(&uuid_rng);
        return;
    }
    status = nxd_uprotocol_connect(&transport, UP_WAIT);
    printf("uProtocol MQTT 5 connect: UCode=%u; telemetry=%s\r\n", status, UPROTOCOL_TELEMETRY_URI);
    /* A failed initial connection is retried by process with backoff. */
    transport_loop();
}

/* Reuse the demo's Wi-Fi, DHCP, DNS and SNTP startup before using uProtocol. */
void mqtt_thread_entry(ULONG parameter)
{
    UINT status;
    NX_PARAMETER_NOT_USED(parameter);
    printf("Starting Eclipse ThreadX uProtocol MQTT 5 demo\r\n");
    tx_mutex_get(&mqtt_i2c_mutex, TX_WAIT_FOREVER);
    powertrain_display_waiting();
    tx_mutex_put(&mqtt_i2c_mutex);
    status = wwd_network_init(WIFI_SSID, WIFI_PASSWORD, WIFI_MODE);
    if (status == NX_SUCCESS)
    {
        status = wwd_network_connect();
    }
    if (status != NX_SUCCESS || sntp_time_get_ms() == 0)
    {
        printf("Network/time initialization failed: 0x%x\r\n", status);
        return;
    }
    mqtt_thread_work(&nx_ip, nx_pool);
}
