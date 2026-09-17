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
#include <string.h>
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

/* Static IP Configuration for Automated Verification Client */
#define CLIENT_IP_ADDRESS       IP_ADDRESS(192, 168, 0, 101)
#define SERVER_IP_ADDRESS       IP_ADDRESS(192, 168, 0, 100)
#define NETWORK_MASK_VAL        IP_ADDRESS(255, 255, 255, 0)
#define GATEWAY_ADDRESS_VAL     IP_ADDRESS(192, 168, 0, 1)

static TX_THREAD                client_thread;
static ULONG                    client_thread_stack[DEMO_STACK_SIZE / sizeof(ULONG)];

static ULONG                    ip_thread_stack[DEMO_STACK_SIZE / sizeof(ULONG)];
static uint8_t                  arp_cache_area[ARP_CACHE_SIZE];

static NX_PACKET_POOL           client_pool;
static NX_IP                    client_ip;

/* Place the NetX Duo packet pool in the NonCacheable section to ensure DMA coherency */
__attribute__((section("NonCacheable"), aligned(64)))
static uint8_t packet_pool_area[PACKET_POOL_SIZE];

/* External hardware driver entry point for NXP i.MX RT ENET MAC */
extern VOID nx_driver_imx(NX_IP_DRIVER *driver_req_ptr);

static void client_thread_entry(ULONG thread_input);

int main(void)
{
    /* Initialize hardware via BSP interface */
    bsp_board_init();

    printf(ANSI_BOLD ANSI_CYAN "\r\n==================================================\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN " Eclipse ThreadX & NetX Duo on NXP i.MX RT1064-EVK\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN " Automated Network Verification Client (Renode)\r\n" ANSI_RESET);
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

    printf(TAG_CLIENT " " MSG_INFO "Initializing NetX Duo System...\r\n" ANSI_RESET);
    nx_system_initialize();

    /* 1. Create packet pool in NonCacheable memory */
    status = nx_packet_pool_create(&client_pool, "Client Packet Pool",
                                   PACKET_SIZE, packet_pool_area, PACKET_POOL_SIZE);
    if (status != NX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_ERROR "Failed to create packet pool: 0x%02X\r\n" ANSI_RESET, status);
        return;
    }
    printf(TAG_CLIENT " " MSG_SUCCESS "Packet pool created (size: %u bytes in NonCacheable memory)\r\n" ANSI_RESET,
           (unsigned int)sizeof(packet_pool_area));

    /* 2. Create IP instance using the NXP i.MX RT ENET driver */
    status = nx_ip_create(&client_ip, "NetX Client IP", CLIENT_IP_ADDRESS,
                          NETWORK_MASK_VAL, &client_pool, nx_driver_imx,
                          ip_thread_stack, DEMO_STACK_SIZE, 1);
    if (status != NX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_ERROR "Failed to create IP instance: 0x%02X\r\n" ANSI_RESET, status);
        return;
    }
    printf(TAG_CLIENT " " MSG_SUCCESS "IP instance created (192.168.0.101)\r\n" ANSI_RESET);

    /* 3. Set Gateway Address */
    nx_ip_gateway_address_set(&client_ip, GATEWAY_ADDRESS_VAL);

    /* 4. Enable ARP */
    printf(TAG_CLIENT " " MSG_INFO "Enabling ARP...\r\n" ANSI_RESET);
    status = nx_arp_enable(&client_ip, (VOID *)arp_cache_area, ARP_CACHE_SIZE);
    if (status != NX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_ERROR "Failed to enable ARP: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* 5. Enable ICMP (Ping) */
    printf(TAG_CLIENT " " MSG_INFO "Enabling ICMP...\r\n" ANSI_RESET);
    status = nx_icmp_enable(&client_ip);
    if (status != NX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_ERROR "Failed to enable ICMP: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* 6. Enable UDP */
    printf(TAG_CLIENT " " MSG_INFO "Enabling UDP...\r\n" ANSI_RESET);
    status = nx_udp_enable(&client_ip);
    if (status != NX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_ERROR "Failed to enable UDP: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* 7. Enable TCP */
    printf(TAG_CLIENT " " MSG_INFO "Enabling TCP...\r\n" ANSI_RESET);
    status = nx_tcp_enable(&client_ip);
    if (status != NX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_ERROR "Failed to enable TCP: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* 8. Start Automated Verification Thread */
    tx_thread_create(&client_thread, "Client Verification Thread", client_thread_entry, 0,
                     client_thread_stack, DEMO_STACK_SIZE, 3, 3, TX_NO_TIME_SLICE, TX_AUTO_START);

    printf(TAG_CLIENT " " MSG_SUCCESS "Verification thread registered.\r\n" ANSI_RESET);
}

static void client_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    ULONG actual_status = 0;
    UINT status;
    int test_ping_passed = 0;
    int test_udp_passed = 0;
    int test_tcp_passed = 0;

    printf(TAG_CLIENT " " MSG_INFO "Bringing Ethernet Link UP...\r\n" ANSI_RESET);
    status = nx_ip_driver_direct_command(&client_ip, NX_LINK_ENABLE, &actual_status);
    if (status == NX_SUCCESS || status == NX_ALREADY_ENABLED)
    {
        printf(TAG_CLIENT " " MSG_SUCCESS "Ethernet link is UP!\r\n" ANSI_RESET);
    }
    else
    {
        printf(TAG_CLIENT " " MSG_WARNING "nx_ip_driver_direct_command NX_LINK_ENABLE status: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* Allow network stack and server node to settle */
    printf(TAG_CLIENT " " MSG_INFO "Waiting for network convergence...\r\n" ANSI_RESET);
    tx_thread_sleep(150);

    printf("\r\n" ANSI_BOLD ANSI_CYAN "==================================================\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN " Starting Multi-Node Network Verification Suite\r\n" ANSI_RESET);
    printf(ANSI_CYAN " Target Echo Server: 192.168.0.100 (Port 7)\r\n" ANSI_RESET);
    printf(ANSI_CYAN " Local Client Node:  192.168.0.101\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN "==================================================\r\n\r\n" ANSI_RESET);

    /* ------------------------------------------------------------------
     * Test 1: ICMP Ping (Echo Request & Reply)
     * ------------------------------------------------------------------ */
    printf(TAG_CLIENT " [Test 1/3] Testing ICMP Ping to 192.168.0.100...\r\n");
    NX_PACKET *ping_response = NX_NULL;
    status = nx_icmp_ping(&client_ip, SERVER_IP_ADDRESS, "ThreadX_Ping", 12, &ping_response, 200);
    if (status == NX_SUCCESS && ping_response != NX_NULL)
    {
        printf(TAG_CLIENT " " MSG_SUCCESS "[PASS] ICMP Ping successful! Response received from 192.168.0.100\r\n" ANSI_RESET);
        nx_packet_release(ping_response);
        test_ping_passed = 1;
    }
    else
    {
        printf(TAG_CLIENT " " MSG_ERROR "[FAIL] ICMP Ping timed out or failed: status 0x%02X\r\n" ANSI_RESET, status);
    }

    /* Small delay between tests */
    tx_thread_sleep(50);

    /* ------------------------------------------------------------------
     * Test 2: UDP Echo (Datagram Tx & Rx on Port 7)
     * ------------------------------------------------------------------ */
    printf("\r\n" TAG_CLIENT " [Test 2/3] Testing UDP Echo on port 7...\r\n");
    NX_UDP_SOCKET udp_client_socket;
    status = nx_udp_socket_create(&client_ip, &udp_client_socket, "Client UDP Socket",
                                  NX_IP_NORMAL, NX_FRAGMENT_OKAY, 0x80, 5);
    if (status == NX_SUCCESS)
    {
        status = nx_udp_socket_bind(&udp_client_socket, NX_ANY_PORT, TX_WAIT_FOREVER);
        if (status == NX_SUCCESS)
        {
            NX_PACKET *tx_packet = NX_NULL;
            if (nx_packet_allocate(&client_pool, &tx_packet, NX_UDP_PACKET, TX_WAIT_FOREVER) == NX_SUCCESS)
            {
                const char *udp_payload = "Hello ThreadX UDP Echo!";
                status = nx_packet_data_append(tx_packet, (VOID *)udp_payload, strlen(udp_payload), &client_pool, TX_WAIT_FOREVER);
                if (status == NX_SUCCESS)
                {
                    printf(TAG_CLIENT " " MSG_INFO "Sent UDP payload: '%s'\r\n" ANSI_RESET, udp_payload);
                    status = nx_udp_socket_send(&udp_client_socket, tx_packet, SERVER_IP_ADDRESS, ECHO_SERVER_PORT);
                }
                if (status != NX_SUCCESS)
                {
                    nx_packet_release(tx_packet);
                    printf(TAG_CLIENT " " MSG_ERROR "[FAIL] Failed to send UDP packet: 0x%02X\r\n" ANSI_RESET, status);
                }
                else
                {
                    NX_PACKET *rx_packet = NX_NULL;
                    status = nx_udp_socket_receive(&udp_client_socket, &rx_packet, 200);
                    if (status == NX_SUCCESS && rx_packet != NX_NULL)
                    {
                        CHAR rx_buf[128];
                        ULONG bytes_copied = 0;

                        /* Bounded extract: a response longer than the buffer is truncated. */
                        nx_packet_data_extract_offset(rx_packet, 0, rx_buf,
                                                      (ULONG)(sizeof(rx_buf) - 1), &bytes_copied);
                        rx_buf[bytes_copied] = '\0';
                        printf(TAG_CLIENT " " MSG_SUCCESS "[PASS] Received UDP Echo: '%s' (%lu bytes)\r\n" ANSI_RESET,
                               rx_buf, bytes_copied);
                        nx_packet_release(rx_packet);
                        test_udp_passed = 1;
                    }
                    else
                    {
                        printf(TAG_CLIENT " " MSG_ERROR "[FAIL] UDP Echo receive timed out or failed: 0x%02X\r\n" ANSI_RESET, status);
                    }
                }
            }
            nx_udp_socket_unbind(&udp_client_socket);
        }
        nx_udp_socket_delete(&udp_client_socket);
    }
    else
    {
        printf(TAG_CLIENT " " MSG_ERROR "[FAIL] Failed to create UDP socket: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* Small delay between tests */
    tx_thread_sleep(50);

    /* ------------------------------------------------------------------
     * Test 3: TCP Echo (Connection, Stream Tx & Rx on Port 7)
     * ------------------------------------------------------------------ */
    printf("\r\n" TAG_CLIENT " [Test 3/3] Testing TCP Echo on port 7...\r\n");
    NX_TCP_SOCKET tcp_client_socket;
    status = nx_tcp_socket_create(&client_ip, &tcp_client_socket, "Client TCP Socket",
                                  NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 512, NX_NULL, NX_NULL);
    if (status == NX_SUCCESS)
    {
        status = nx_tcp_client_socket_bind(&tcp_client_socket, NX_ANY_PORT, TX_WAIT_FOREVER);
        if (status == NX_SUCCESS)
        {
            printf(TAG_CLIENT " " MSG_INFO "Connecting to 192.168.0.100:7...\r\n" ANSI_RESET);
            status = nx_tcp_client_socket_connect(&tcp_client_socket, SERVER_IP_ADDRESS, ECHO_SERVER_PORT, 200);
            if (status == NX_SUCCESS)
            {
                printf(TAG_CLIENT " " MSG_SUCCESS "TCP Connected! Sending stream payload...\r\n" ANSI_RESET);
                NX_PACKET *tx_packet = NX_NULL;
                if (nx_packet_allocate(&client_pool, &tx_packet, NX_TCP_PACKET, TX_WAIT_FOREVER) == NX_SUCCESS)
                {
                    const char *tcp_payload = "Hello ThreadX TCP Echo!";
                    status = nx_packet_data_append(tx_packet, (VOID *)tcp_payload, strlen(tcp_payload), &client_pool, TX_WAIT_FOREVER);
                    if (status == NX_SUCCESS)
                    {
                        printf(TAG_CLIENT " " MSG_INFO "Sent TCP payload: '%s'\r\n" ANSI_RESET, tcp_payload);
                        status = nx_tcp_socket_send(&tcp_client_socket, tx_packet, 200);
                    }
                    if (status != NX_SUCCESS)
                    {
                        nx_packet_release(tx_packet);
                        printf(TAG_CLIENT " " MSG_ERROR "[FAIL] Failed to send TCP packet: 0x%02X\r\n" ANSI_RESET, status);
                    }
                    else
                    {
                        NX_PACKET *rx_packet = NX_NULL;
                        status = nx_tcp_socket_receive(&tcp_client_socket, &rx_packet, 200);
                        if (status == NX_SUCCESS && rx_packet != NX_NULL)
                        {
                            CHAR rx_buf[128];
                            ULONG bytes_copied = 0;

                            /* Bounded extract: a response longer than the buffer is truncated. */
                            nx_packet_data_extract_offset(rx_packet, 0, rx_buf,
                                                          (ULONG)(sizeof(rx_buf) - 1), &bytes_copied);
                            rx_buf[bytes_copied] = '\0';
                            printf(TAG_CLIENT " " MSG_SUCCESS "[PASS] Received TCP Echo: '%s' (%lu bytes)\r\n" ANSI_RESET,
                                   rx_buf, bytes_copied);
                            nx_packet_release(rx_packet);
                            test_tcp_passed = 1;
                        }
                        else
                        {
                            printf(TAG_CLIENT " " MSG_ERROR "[FAIL] TCP Echo receive timed out or failed: 0x%02X\r\n" ANSI_RESET, status);
                        }
                    }
                }
                nx_tcp_socket_disconnect(&tcp_client_socket, 100);
            }
            else
            {
                printf(TAG_CLIENT " " MSG_ERROR "[FAIL] TCP connect to 192.168.0.100:7 failed: 0x%02X\r\n" ANSI_RESET, status);
            }
            nx_tcp_client_socket_unbind(&tcp_client_socket);
        }
        nx_tcp_socket_delete(&tcp_client_socket);
    }
    else
    {
        printf(TAG_CLIENT " " MSG_ERROR "[FAIL] Failed to create TCP socket: 0x%02X\r\n" ANSI_RESET, status);
    }

    /* ------------------------------------------------------------------
     * Verification Summary
     * ------------------------------------------------------------------ */
    printf("\r\n" ANSI_BOLD "==================================================\r\n" ANSI_RESET);
    if (test_ping_passed && test_udp_passed && test_tcp_passed)
    {
        printf(ANSI_BOLD ANSI_GREEN " [VERIFICATION SUCCESS] ALL NETWORK TESTS PASSED!\r\n" ANSI_RESET);
        printf(ANSI_GREEN "  - [PASS] ICMP Ping (Echo Request & Reply)\r\n" ANSI_RESET);
        printf(ANSI_GREEN "  - [PASS] UDP Echo (Datagram Tx & Rx)\r\n" ANSI_RESET);
        printf(ANSI_GREEN "  - [PASS] TCP Echo (Connection, Stream Tx & Rx)\r\n" ANSI_RESET);
    }
    else
    {
        printf(ANSI_BOLD ANSI_RED " [VERIFICATION INCOMPLETE] SOME TESTS FAILED!\r\n" ANSI_RESET);
        if (!test_ping_passed) printf(ANSI_RED "  - [FAIL] ICMP Ping\r\n" ANSI_RESET);
        if (!test_udp_passed)  printf(ANSI_RED "  - [FAIL] UDP Echo\r\n" ANSI_RESET);
        if (!test_tcp_passed)  printf(ANSI_RED "  - [FAIL] TCP Echo\r\n" ANSI_RESET);
    }
    printf(ANSI_BOLD "==================================================\r\n\r\n" ANSI_RESET);

    /* Heartbeat loop */
    while (1)
    {
        tx_thread_sleep(50);
        bsp_led_toggle();
    }
}
