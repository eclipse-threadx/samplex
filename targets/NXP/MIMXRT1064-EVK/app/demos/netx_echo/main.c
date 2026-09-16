/* 
 * Copyright (c) 2026 Eclipse ThreadX contributors
 * 
 *  This program and the accompanying materials are made available 
 *  under the terms of the MIT license which is available at
 *  https://opensource.org/license/mit.
 * 
 *  SPDX-License-Identifier: MIT
 * 
 *  Contributors:
 *     Ali Eissa - 2026 version.
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
#include "nx_api.h"
#include "ansi_colors.h"

#define DEMO_STACK_SIZE         2048
#define PACKET_SIZE             1536
#define PACKET_POOL_SIZE        (PACKET_SIZE * 24)
#define ECHO_SERVER_PORT        7
#define ARP_CACHE_SIZE          1024

/* Static IP Configuration for Renode Simulation & Physical Testing */
#define IP_ADDRESS_VAL          IP_ADDRESS(192, 168, 0, 100)
#define NETWORK_MASK_VAL        IP_ADDRESS(255, 255, 255, 0)
#define GATEWAY_ADDRESS_VAL     IP_ADDRESS(192, 168, 0, 1)

static TX_THREAD                monitor_thread;
static ULONG                    monitor_thread_stack[DEMO_STACK_SIZE / sizeof(ULONG)];

static TX_THREAD                udp_echo_thread;
static ULONG                    udp_echo_thread_stack[DEMO_STACK_SIZE / sizeof(ULONG)];

static TX_THREAD                tcp_echo_thread;
static ULONG                    tcp_echo_thread_stack[DEMO_STACK_SIZE / sizeof(ULONG)];

static ULONG                    ip_thread_stack[DEMO_STACK_SIZE / sizeof(ULONG)];
static uint8_t                  arp_cache_area[ARP_CACHE_SIZE];

static NX_PACKET_POOL           pool_0;
static NX_IP                    ip_0;

/* Place the NetX Duo packet pool in the NonCacheable section to ensure DMA coherency */
__attribute__((section("NonCacheable"), aligned(64)))
static uint8_t packet_pool_area[PACKET_POOL_SIZE];

/* External hardware driver entry point for NXP i.MX RT ENET MAC */
extern VOID nx_driver_imx(NX_IP_DRIVER *driver_req_ptr);

static void monitor_thread_entry(ULONG thread_input);
static void udp_echo_thread_entry(ULONG thread_input);
static void tcp_echo_thread_entry(ULONG thread_input);

int main(void)
{
    /* Initialize hardware via BSP interface */
    bsp_board_init();

    printf(ANSI_BOLD ANSI_CYAN "\r\n==================================================\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN " Eclipse ThreadX & NetX Duo on NXP i.MX RT1064-EVK\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN " Virtual Ethernet Networking & Echo Demo (Renode)\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN "==================================================\r\n\r\n" ANSI_RESET);

    printf(TAG_SYSTEM " " MSG_INFO "Core Clock: %lu MHz | Tick Rate: %u Hz\r\n" ANSI_RESET,
           SystemCoreClock / 1000000UL, TX_TIMER_TICKS_PER_SECOND);

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
    printf(TAG_NETWORK " " MSG_SUCCESS "Packet pool created (size: %u bytes in NonCacheable memory)\r\n" ANSI_RESET,
           (unsigned int)sizeof(packet_pool_area));

    /* 2. Create IP instance using the NXP i.MX RT ENET driver */
    status = nx_ip_create(&ip_0, "NetX IP Instance 0", IP_ADDRESS_VAL,
                          NETWORK_MASK_VAL, &pool_0, nx_driver_imx,
                          ip_thread_stack, DEMO_STACK_SIZE, 1);
    if (status != NX_SUCCESS)
    {
        printf(TAG_NETWORK " " MSG_ERROR "Failed to create IP instance: 0x%02X\r\n" ANSI_RESET, status);
        return;
    }
    printf(TAG_NETWORK " " MSG_SUCCESS "IP instance created\r\n" ANSI_RESET);

    /* 3. Set Gateway Address */
    nx_ip_gateway_address_set(&ip_0, GATEWAY_ADDRESS_VAL);

    /* 4. Enable ARP */
    printf(TAG_NETWORK " " MSG_INFO "Enabling ARP...\r\n" ANSI_RESET);
    status = nx_arp_enable(&ip_0, (VOID *)arp_cache_area, ARP_CACHE_SIZE);
    if (status != NX_SUCCESS)
    {
        printf(TAG_NETWORK " " MSG_ERROR "Failed to enable ARP: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* 5. Enable ICMP (Ping) */
    printf(TAG_NETWORK " " MSG_INFO "Enabling ICMP (Ping responder)...\r\n" ANSI_RESET);
    status = nx_icmp_enable(&ip_0);
    if (status != NX_SUCCESS)
    {
        printf(TAG_NETWORK " " MSG_ERROR "Failed to enable ICMP: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* 6. Enable UDP */
    printf(TAG_NETWORK " " MSG_INFO "Enabling UDP...\r\n" ANSI_RESET);
    status = nx_udp_enable(&ip_0);
    if (status != NX_SUCCESS)
    {
        printf(TAG_NETWORK " " MSG_ERROR "Failed to enable UDP: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* 7. Enable TCP */
    printf(TAG_NETWORK " " MSG_INFO "Enabling TCP...\r\n" ANSI_RESET);
    status = nx_tcp_enable(&ip_0);
    if (status != NX_SUCCESS)
    {
        printf(TAG_NETWORK " " MSG_ERROR "Failed to enable TCP: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* 8. Start Monitor / Heartbeat Thread */
    tx_thread_create(&monitor_thread, "Network Monitor", monitor_thread_entry, 0,
                     monitor_thread_stack, DEMO_STACK_SIZE, 3, 3, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* 9. Start UDP Echo Server Thread */
    tx_thread_create(&udp_echo_thread, "UDP Echo Thread", udp_echo_thread_entry, 0,
                     udp_echo_thread_stack, DEMO_STACK_SIZE, 4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* 10. Start TCP Echo Server Thread */
    tx_thread_create(&tcp_echo_thread, "TCP Echo Thread", tcp_echo_thread_entry, 0,
                     tcp_echo_thread_stack, DEMO_STACK_SIZE, 4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    printf(TAG_NETWORK " " MSG_SUCCESS "All network threads registered successfully.\r\n" ANSI_RESET);
}

static void monitor_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    ULONG ip_address = 0;
    ULONG network_mask = 0;
    ULONG actual_status = 0;
    uint8_t led_state = 0;

    printf(TAG_NETWORK " " MSG_INFO "Bringing Ethernet Link UP...\r\n" ANSI_RESET);
    UINT status = nx_ip_driver_direct_command(&ip_0, NX_LINK_ENABLE, &actual_status);
    if (status == NX_SUCCESS || status == NX_ALREADY_ENABLED)
    {
        printf(TAG_NETWORK " " MSG_SUCCESS "Ethernet link is UP!\r\n" ANSI_RESET);
    }
    else
    {
        printf(TAG_NETWORK " " MSG_WARNING "nx_ip_driver_direct_command NX_LINK_ENABLE status: 0x%02X\r\n" ANSI_RESET, status);
    }

    nx_ip_address_get(&ip_0, &ip_address, &network_mask);
    printf("\r\n" ANSI_BOLD ANSI_GREEN "================ Network Ready ================\r\n" ANSI_RESET);
    printf(ANSI_GREEN "  Static IPv4 : %lu.%lu.%lu.%lu\r\n" ANSI_RESET,
           (ip_address >> 24) & 0xFF, (ip_address >> 16) & 0xFF,
           (ip_address >> 8) & 0xFF, ip_address & 0xFF);
    printf(ANSI_GREEN "  Subnet Mask : %lu.%lu.%lu.%lu\r\n" ANSI_RESET,
           (network_mask >> 24) & 0xFF, (network_mask >> 16) & 0xFF,
           (network_mask >> 8) & 0xFF, network_mask & 0xFF);
    printf(ANSI_GREEN "  Services    : ICMP Ping, UDP Echo (Port 7), TCP Echo (Port 7)\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_GREEN "===============================================\r\n\r\n" ANSI_RESET);

    while (1)
    {
        /* Sleep 500 ms (50 ticks) */
        tx_thread_sleep(50);

        /* Toggle User LED to indicate active heartbeat */
        bsp_led_toggle();
        led_state = !led_state;
    }
}

static void udp_echo_thread_entry(ULONG thread_input)
{
    NX_UDP_SOCKET udp_socket;
    NX_PACKET *rx_packet;
    UINT status;

    (void)thread_input;

    status = nx_udp_socket_create(&ip_0, &udp_socket, "UDP Echo Socket",
                                  NX_IP_NORMAL, NX_FRAGMENT_OKAY, 0x80, 5);
    if (status != NX_SUCCESS)
    {
        printf(TAG_ECHO " " MSG_ERROR "Failed to create UDP socket: 0x%02X\r\n" ANSI_RESET, status);
        return;
    }

    status = nx_udp_socket_bind(&udp_socket, ECHO_SERVER_PORT, TX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        printf(TAG_ECHO " " MSG_ERROR "Failed to bind UDP port %u: 0x%02X\r\n" ANSI_RESET, ECHO_SERVER_PORT, status);
        nx_udp_socket_delete(&udp_socket);
        return;
    }

    printf(TAG_ECHO " " MSG_INFO "UDP Echo Server listening on port %d\r\n" ANSI_RESET, ECHO_SERVER_PORT);

    while (1)
    {
        status = nx_udp_socket_receive(&udp_socket, &rx_packet, TX_WAIT_FOREVER);
        if (status == NX_SUCCESS)
        {
            ULONG peer_ip = 0;
            UINT peer_port = 0;
            nx_udp_source_extract(rx_packet, &peer_ip, &peer_port);

            printf(TAG_ECHO " " MSG_SUCCESS "UDP Rx from %lu.%lu.%lu.%lu:%u (%lu bytes), echoing...\r\n" ANSI_RESET,
                   (peer_ip >> 24) & 0xFF, (peer_ip >> 16) & 0xFF,
                   (peer_ip >> 8) & 0xFF, peer_ip & 0xFF,
                   peer_port, rx_packet->nx_packet_length);

            /* Allocate a response packet from the pool */
            NX_PACKET *tx_packet = NX_NULL;
            if (nx_packet_allocate(&pool_0, &tx_packet, NX_UDP_PACKET, TX_NO_WAIT) == NX_SUCCESS)
            {
                CHAR echo_buf[512];
                ULONG bytes_copied = 0;
                if (rx_packet->nx_packet_length <= sizeof(echo_buf) &&
                    nx_packet_data_retrieve(rx_packet, echo_buf, &bytes_copied) == NX_SUCCESS)
                {
                    status = nx_packet_data_append(tx_packet, echo_buf, bytes_copied, &pool_0, TX_NO_WAIT);
                    if (status == NX_SUCCESS)
                    {
                        status = nx_udp_socket_send(&udp_socket, tx_packet, peer_ip, peer_port);
                    }
                    if (status != NX_SUCCESS)
                    {
                        nx_packet_release(tx_packet);
                    }
                }
                else
                {
                    nx_packet_release(tx_packet);
                }
            }

            /* Release the received packet */
            nx_packet_release(rx_packet);
        }
    }
}

static void tcp_echo_thread_entry(ULONG thread_input)
{
    NX_TCP_SOCKET echo_socket;
    NX_PACKET *packet_ptr;
    UINT status;

    (void)thread_input;

    status = nx_tcp_socket_create(&ip_0, &echo_socket, "TCP Echo Socket",
                                  NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE,
                                  512, NX_NULL, NX_NULL);
    if (status != NX_SUCCESS)
    {
        printf(TAG_ECHO " " MSG_ERROR "Failed to create TCP socket: 0x%02X\r\n" ANSI_RESET, status);
        return;
    }

    printf(TAG_ECHO " " MSG_INFO "TCP Echo Server listening on port %d\r\n" ANSI_RESET, ECHO_SERVER_PORT);

    while (1)
    {
        status = nx_tcp_server_socket_listen(&ip_0, ECHO_SERVER_PORT, &echo_socket, 5, NX_NULL);
        if (status != NX_SUCCESS)
        {
            nx_tcp_server_socket_unlisten(&ip_0, ECHO_SERVER_PORT);
            tx_thread_sleep(10);
            continue;
        }

        if (nx_tcp_server_socket_accept(&echo_socket, NX_WAIT_FOREVER) == NX_SUCCESS)
        {
            printf(TAG_ECHO " " MSG_SUCCESS "TCP Client connected.\r\n" ANSI_RESET);

            while (nx_tcp_socket_receive(&echo_socket, &packet_ptr, NX_WAIT_FOREVER) == NX_SUCCESS)
            {
                printf(TAG_ECHO " " MSG_SUCCESS "TCP Rx %lu bytes, echoing...\r\n" ANSI_RESET,
                       packet_ptr->nx_packet_length);
                NX_PACKET *tx_packet = NX_NULL;
                if (nx_packet_allocate(&pool_0, &tx_packet, NX_TCP_PACKET, TX_WAIT_FOREVER) == NX_SUCCESS)
                {
                    CHAR echo_buf[512];
                    ULONG bytes_copied = 0;
                    if (packet_ptr->nx_packet_length <= sizeof(echo_buf) &&
                        nx_packet_data_retrieve(packet_ptr, echo_buf, &bytes_copied) == NX_SUCCESS)
                    {
                        status = nx_packet_data_append(tx_packet, echo_buf, bytes_copied, &pool_0, TX_WAIT_FOREVER);
                        if (status == NX_SUCCESS)
                        {
                            status = nx_tcp_socket_send(&echo_socket, tx_packet, TX_WAIT_FOREVER);
                        }
                        if (status != NX_SUCCESS)
                        {
                            nx_packet_release(tx_packet);
                        }
                    }
                    else
                    {
                        nx_packet_release(tx_packet);
                    }
                }
                nx_packet_release(packet_ptr);
            }

            printf(TAG_ECHO " " MSG_WARNING "TCP Client disconnected.\r\n" ANSI_RESET);
            nx_tcp_socket_disconnect(&echo_socket, NX_WAIT_FOREVER);
            nx_tcp_server_socket_unaccept(&echo_socket);
        }
        nx_tcp_server_socket_unlisten(&ip_0, ECHO_SERVER_PORT);
    }
}
