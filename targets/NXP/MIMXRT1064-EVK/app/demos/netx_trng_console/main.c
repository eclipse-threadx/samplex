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

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "bsp/board.h"
#include "bsp/led.h"
#include "bsp/console.h"
#include "board_config.h"
#include "ansi_colors.h"
#include "trng.h"
#include "tx_api.h"
#include "nx_api.h"

#define DEMO_STACK_SIZE          2048
#define PACKET_SIZE              1536
#define PACKET_POOL_SIZE         ((PACKET_SIZE + sizeof(NX_PACKET)) * 24)
#define ARP_CACHE_SIZE           512
#define CONSOLE_SERVER_PORT      23

#define IP_ADDRESS_VAL           IP_ADDRESS(192, 168, 0, 100)
#define NETWORK_MASK_VAL         IP_ADDRESS(255, 255, 255, 0)
#define GATEWAY_ADDRESS_VAL      IP_ADDRESS(192, 168, 0, 1)

#define TAG_SHELL                "\x1b[38;5;243m[Shell]"
#define TAG_TRNG                 "\x1b[38;5;243m[TRNG]"

/* Memory buffers */
static ULONG ip_thread_stack[DEMO_STACK_SIZE / sizeof(ULONG)];
static ULONG heartbeat_thread_stack[DEMO_STACK_SIZE / sizeof(ULONG)];
static ULONG shell_thread_stack[DEMO_STACK_SIZE / sizeof(ULONG)];
static ULONG arp_cache_area[ARP_CACHE_SIZE / sizeof(ULONG)];

__attribute__((section(".NonCacheable")))
static uint8_t packet_pool_area[PACKET_POOL_SIZE];

/* NetX Duo & ThreadX objects */
static NX_PACKET_POOL pool_0;
static NX_IP ip_0;
static TX_THREAD heartbeat_thread;
static TX_THREAD shell_thread;

/* External driver entry point */
VOID nx_driver_imx(NX_IP_DRIVER *driver_req_ptr);

/* Thread prototypes */
static void heartbeat_thread_entry(ULONG thread_input);
static void shell_thread_entry(ULONG thread_input);

int main(void)
{
    /* Initialize hardware via BSP interface */
    bsp_board_init();

    /* Initialize on-chip Hardware TRNG */
    trng_init();

    printf(ANSI_BOLD ANSI_CYAN "\r\n==================================================\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN " Eclipse ThreadX & NetX Duo on NXP i.MX RT1064-EVK\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN " Hardware TRNG & Network Diagnostic Shell (Renode)\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN "==================================================\r\n\r\n" ANSI_RESET);

    printf(TAG_SYSTEM " " MSG_INFO "Core Clock: %lu MHz | Tick Rate: %u Hz\r\n" ANSI_RESET,
           SystemCoreClock / 1000000UL, TX_TIMER_TICKS_PER_SECOND);
    printf(TAG_TRNG " " MSG_INFO "On-chip True Random Number Generator initialized @ 0x400CC000\r\n" ANSI_RESET);

    /* Enter ThreadX kernel */
    tx_kernel_enter();

    return 0;
}

void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;
    UINT status;

    printf(TAG_NETWORK " " MSG_INFO "Initializing NetX Duo System...\r\n" ANSI_RESET);
    nx_system_initialize();

    /* 1. Create packet pool in NonCacheable memory */
    status = nx_packet_pool_create(&pool_0, "NetX Main Packet Pool",
                                   PACKET_SIZE, packet_pool_area, PACKET_POOL_SIZE);
    if (status != NX_SUCCESS)
    {
        printf(TAG_NETWORK " " MSG_ERROR "Failed to create packet pool: 0x%02X\r\n" ANSI_RESET, status);
        return;
    }
    printf(TAG_NETWORK " " MSG_SUCCESS "Packet pool created (%u bytes in NonCacheable memory)\r\n" ANSI_RESET,
           (unsigned int)sizeof(packet_pool_area));

    /* 2. Create IP instance */
    status = nx_ip_create(&ip_0, "NetX IP Instance 0", IP_ADDRESS_VAL,
                          NETWORK_MASK_VAL, &pool_0, nx_driver_imx,
                          ip_thread_stack, DEMO_STACK_SIZE, 1);
    if (status != NX_SUCCESS)
    {
        printf(TAG_NETWORK " " MSG_ERROR "Failed to create IP instance: 0x%02X\r\n" ANSI_RESET, status);
        return;
    }
    printf(TAG_NETWORK " " MSG_SUCCESS "IP instance created\r\n" ANSI_RESET);

    /* 3. Gateway & Services */
    nx_ip_gateway_address_set(&ip_0, GATEWAY_ADDRESS_VAL);

    printf(TAG_NETWORK " " MSG_INFO "Enabling ARP...\r\n" ANSI_RESET);
    nx_arp_enable(&ip_0, (VOID *)arp_cache_area, ARP_CACHE_SIZE);

    printf(TAG_NETWORK " " MSG_INFO "Enabling ICMP (Ping responder)...\r\n" ANSI_RESET);
    nx_icmp_enable(&ip_0);

    printf(TAG_NETWORK " " MSG_INFO "Enabling TCP...\r\n" ANSI_RESET);
    nx_tcp_enable(&ip_0);

    /* 4. Create Heartbeat Thread */
    status = tx_thread_create(&heartbeat_thread, "Heartbeat Thread",
                              heartbeat_thread_entry, 0,
                              heartbeat_thread_stack, DEMO_STACK_SIZE,
                              15, 15, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        printf(TAG_SYSTEM " " MSG_ERROR "Failed to create Heartbeat thread: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* 5. Create TRNG Console Shell Thread */
    status = tx_thread_create(&shell_thread, "TRNG Shell Thread",
                              shell_thread_entry, 0,
                              shell_thread_stack, DEMO_STACK_SIZE,
                              10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        printf(TAG_SYSTEM " " MSG_ERROR "Failed to create Shell thread: 0x%02X\r\n" ANSI_RESET, status);
    }

    printf(TAG_NETWORK " " MSG_SUCCESS "Network threads registered successfully.\r\n" ANSI_RESET);
}

static void heartbeat_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    UINT status;
    ULONG actual_status;
    ULONG ip_address, network_mask;

    printf(TAG_NETWORK " " MSG_INFO "Bringing Ethernet Link UP...\r\n" ANSI_RESET);
    status = nx_ip_driver_direct_command(&ip_0, NX_LINK_ENABLE, &actual_status);
    if (status == NX_SUCCESS || status == NX_ALREADY_ENABLED)
    {
        printf(TAG_NETWORK " " MSG_SUCCESS "Ethernet link is UP!\r\n" ANSI_RESET);
    }
    else
    {
        printf(TAG_NETWORK " " MSG_ERROR "nx_ip_driver_direct_command NX_LINK_ENABLE status: 0x%02X\r\n" ANSI_RESET, status);
    }

    nx_ip_address_get(&ip_0, &ip_address, &network_mask);
    printf("\r\n" ANSI_BOLD ANSI_GREEN "================ Network Ready ================\r\n" ANSI_RESET);
    printf(ANSI_GREEN "  Static IPv4 : %lu.%lu.%lu.%lu\r\n" ANSI_RESET,
           (ip_address >> 24) & 0xFF, (ip_address >> 16) & 0xFF,
           (ip_address >> 8) & 0xFF, ip_address & 0xFF);
    printf(ANSI_GREEN "  Subnet Mask : %lu.%lu.%lu.%lu\r\n" ANSI_RESET,
           (network_mask >> 24) & 0xFF, (network_mask >> 16) & 0xFF,
           (network_mask >> 8) & 0xFF, network_mask & 0xFF);
    printf(ANSI_GREEN "  Services    : ICMP Ping, Hardware TRNG Shell (TCP Port %d)\r\n" ANSI_RESET, CONSOLE_SERVER_PORT);
    printf(ANSI_BOLD ANSI_GREEN "===============================================\r\n\r\n" ANSI_RESET);

    while (1)
    {
        tx_thread_sleep(50);
        bsp_led_toggle();
    }
}

static void send_tcp_response(NX_TCP_SOCKET *socket, const char *msg)
{
    NX_PACKET *tx_packet = NX_NULL;
    UINT status;
    size_t len = strlen(msg);

    status = nx_packet_allocate(&pool_0, &tx_packet, NX_TCP_PACKET, TX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        return;
    }

    status = nx_packet_data_append(tx_packet, (VOID *)msg, len, &pool_0, TX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        nx_packet_release(tx_packet);
        return;
    }

    status = nx_tcp_socket_send(socket, tx_packet, 200);
    if (status != NX_SUCCESS)
    {
        /* In NetX Duo, a failed send leaves packet ownership with the caller */
        nx_packet_release(tx_packet);
    }
}

static void shell_thread_entry(ULONG thread_input)
{
    NX_TCP_SOCKET shell_socket;
    NX_PACKET *packet_ptr;
    UINT status;
    char line_buffer[128];
    char resp_buffer[256];

    (void)thread_input;

    status = nx_tcp_socket_create(&ip_0, &shell_socket, "TRNG Shell Socket",
                                  NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE,
                                  1024, NX_NULL, NX_NULL);
    if (status != NX_SUCCESS)
    {
        printf(TAG_SHELL " " MSG_ERROR "Failed to create TCP socket: 0x%02X\r\n" ANSI_RESET, status);
        return;
    }

    printf(TAG_SHELL " " MSG_INFO "TRNG Diagnostic Shell listening on port %d\r\n" ANSI_RESET, CONSOLE_SERVER_PORT);

    while (1)
    {
        status = nx_tcp_server_socket_listen(&ip_0, CONSOLE_SERVER_PORT, &shell_socket, 5, NX_NULL);
        if (status != NX_SUCCESS)
        {
            nx_tcp_server_socket_unlisten(&ip_0, CONSOLE_SERVER_PORT);
            tx_thread_sleep(10);
            continue;
        }

        status = nx_tcp_server_socket_accept(&shell_socket, NX_WAIT_FOREVER);
        if (status == NX_SUCCESS)
        {
            ULONG peer_ip = 0;
            ULONG peer_port = 0;
            nx_tcp_socket_peer_info_get(&shell_socket, &peer_ip, &peer_port);

            printf(TAG_SHELL " " MSG_SUCCESS "Client connected from %lu.%lu.%lu.%lu:%lu\r\n" ANSI_RESET,
                   (peer_ip >> 24) & 0xFF, (peer_ip >> 16) & 0xFF,
                   (peer_ip >> 8) & 0xFF, peer_ip & 0xFF, peer_port);

            /* Send Welcome Banner */
            send_tcp_response(&shell_socket,
                "\r\n==================================================\r\n"
                " NXP i.MX RT1064-EVK Hardware TRNG Console\r\n"
                " Eclipse ThreadX & NetX Duo Management Shell\r\n"
                "==================================================\r\n"
                "Type 'help' for available commands.\r\n\r\nmimxrt1064> ");

            while (1)
            {
                status = nx_tcp_socket_receive(&shell_socket, &packet_ptr, NX_WAIT_FOREVER);
                if (status != NX_SUCCESS)
                {
                    break;
                }

                ULONG bytes_copied = 0;

                /* nx_packet_data_retrieve copies the whole chain and takes no
                   destination size, so an over-long line is refused, not truncated. */
                if (packet_ptr->nx_packet_length >= sizeof(line_buffer))
                {
                    nx_packet_release(packet_ptr);
                    send_tcp_response(&shell_socket,
                        "[ERROR] Command line too long\r\n\r\nmimxrt1064> ");
                    continue;
                }

                status = nx_packet_data_retrieve(packet_ptr, line_buffer, &bytes_copied);
                nx_packet_release(packet_ptr);

                if (status != NX_SUCCESS && bytes_copied == 0)
                {
                    continue;
                }

                line_buffer[bytes_copied] = '\0';

                /* Safe trimming of trailing CRLF and spaces without pointer underflow */
                size_t len = strlen(line_buffer);
                while (len > 0 && (line_buffer[len - 1] == '\r' ||
                                   line_buffer[len - 1] == '\n' ||
                                   line_buffer[len - 1] == ' '))
                {
                    line_buffer[--len] = '\0';
                }

                if (strlen(line_buffer) == 0)
                {
                    send_tcp_response(&shell_socket, "mimxrt1064> ");
                    continue;
                }

                printf(TAG_SHELL " Received command: '%s'\r\n", line_buffer);

                if (strcmp(line_buffer, "help") == 0)
                {
                    send_tcp_response(&shell_socket,
                        "Available commands:\r\n"
                        "  trng              - Read 4x 32-bit hardware entropy words from on-chip TRNG\r\n"
                        "  info              - Print processor clock, memory, and ThreadX ticks\r\n"
                        "  led on|off|toggle - Control or toggle User LED D18\r\n"
                        "  ping              - Connection health check\r\n"
                        "  quit              - Terminate console session\r\n\r\nmimxrt1064> ");
                }
                else if (strcmp(line_buffer, "trng") == 0 || strcmp(line_buffer, "rand") == 0)
                {
                    uint32_t r1 = 0, r2 = 0, r3 = 0, r4 = 0;
                    int s1 = trng_get_random_u32(&r1);
                    int s2 = trng_get_random_u32(&r2);
                    int s3 = trng_get_random_u32(&r3);
                    int s4 = trng_get_random_u32(&r4);

                    if (s1 != 0 || s2 != 0 || s3 != 0 || s4 != 0)
                    {
                        snprintf(resp_buffer, sizeof(resp_buffer),
                                 "[TRNG] Error: Entropy generation failed (status: %d, %d, %d, %d)\r\n\r\nmimxrt1064> ",
                                 s1, s2, s3, s4);
                        printf(TAG_TRNG " " MSG_ERROR "Entropy generation failed (status: %d, %d, %d, %d)\r\n",
                               s1, s2, s3, s4);
                        send_tcp_response(&shell_socket, resp_buffer);
                    }
                    else
                    {
                        snprintf(resp_buffer, sizeof(resp_buffer),
                                 "[TRNG] Hardware Entropy: 0x%08lX 0x%08lX 0x%08lX 0x%08lX\r\n\r\nmimxrt1064> ",
                                 (unsigned long)r1, (unsigned long)r2, (unsigned long)r3, (unsigned long)r4);
                        printf(TAG_TRNG " Generated entropy: 0x%08lX 0x%08lX 0x%08lX 0x%08lX\r\n",
                               (unsigned long)r1, (unsigned long)r2, (unsigned long)r3, (unsigned long)r4);
                        send_tcp_response(&shell_socket, resp_buffer);
                    }
                }
                else if (strcmp(line_buffer, "info") == 0)
                {
                    snprintf(resp_buffer, sizeof(resp_buffer),
                             "[INFO] Target: NXP MIMXRT1064-EVK (ARM Cortex-M7 @ 600 MHz)\r\n"
                             "[INFO] RTOS: Eclipse ThreadX | Uptime: %lu ticks\r\n"
                             "[INFO] TRNG: On-chip hardware entropy engine active @ 0x400CC000\r\n\r\nmimxrt1064> ",
                             tx_time_get());
                    send_tcp_response(&shell_socket, resp_buffer);
                }
                else if (strncmp(line_buffer, "led", 3) == 0)
                {
                    if (strstr(line_buffer, "on"))
                    {
                        bsp_led_on();
                        send_tcp_response(&shell_socket, "[LED] State: ON\r\n\r\nmimxrt1064> ");
                    }
                    else if (strstr(line_buffer, "off"))
                    {
                        bsp_led_off();
                        send_tcp_response(&shell_socket, "[LED] State: OFF\r\n\r\nmimxrt1064> ");
                    }
                    else
                    {
                        bsp_led_toggle();
                        send_tcp_response(&shell_socket, "[LED] State: TOGGLED\r\n\r\nmimxrt1064> ");
                    }
                }
                else if (strcmp(line_buffer, "ping") == 0)
                {
                    send_tcp_response(&shell_socket, "[PONG] Network connection alive\r\n\r\nmimxrt1064> ");
                }
                else if (strcmp(line_buffer, "quit") == 0 || strcmp(line_buffer, "exit") == 0)
                {
                    send_tcp_response(&shell_socket, "Goodbye!\r\n");
                    break;
                }
                else
                {
                    send_tcp_response(&shell_socket, "Unknown command. Type 'help' for options.\r\n\r\nmimxrt1064> ");
                }
            }

            printf(TAG_SHELL " Client disconnected\r\n");
            nx_tcp_socket_disconnect(&shell_socket, 10);
            nx_tcp_server_socket_unaccept(&shell_socket);
        }

        nx_tcp_server_socket_unlisten(&ip_0, CONSOLE_SERVER_PORT);
    }
}
