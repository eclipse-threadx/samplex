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
#include <string.h>
#include "bsp/board.h"
#include "bsp/led.h"
#include "bsp/console.h"
#include "board_config.h"
#include "ansi_colors.h"
#include "tx_api.h"
#include "nx_api.h"

#define DEMO_STACK_SIZE          2048
#define PACKET_SIZE              1536
#define PACKET_POOL_SIZE         ((PACKET_SIZE + sizeof(NX_PACKET)) * 24)
#define ARP_CACHE_SIZE           512
#define CONSOLE_SERVER_PORT      23

#define CLIENT_IP_ADDRESS_VAL    IP_ADDRESS(192, 168, 0, 101)
#define SERVER_IP_ADDRESS        IP_ADDRESS(192, 168, 0, 100)
#define NETWORK_MASK_VAL         IP_ADDRESS(255, 255, 255, 0)
#define GATEWAY_ADDRESS_VAL      IP_ADDRESS(192, 168, 0, 1)



static ULONG client_ip_stack[DEMO_STACK_SIZE / sizeof(ULONG)];
static ULONG client_test_stack[DEMO_STACK_SIZE / sizeof(ULONG)];
static ULONG client_arp_cache[ARP_CACHE_SIZE / sizeof(ULONG)];

__attribute__((section(".NonCacheable")))
static uint8_t client_packet_pool_area[PACKET_POOL_SIZE];

static NX_PACKET_POOL client_pool;
static NX_IP client_ip;
static TX_THREAD client_test_thread;

VOID nx_driver_imx(NX_IP_DRIVER *driver_req_ptr);
static void client_test_thread_entry(ULONG thread_input);

int main(void)
{
    bsp_board_init();

    printf(ANSI_BOLD ANSI_YELLOW "\r\n==================================================\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_YELLOW " MIMXRT1064 TRNG & Console Verification Client\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_YELLOW " Running on Simulated Node 2 (192.168.0.101)\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_YELLOW "==================================================\r\n\r\n" ANSI_RESET);

    tx_kernel_enter();

    return 0;
}

void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;
    UINT status;

    nx_system_initialize();

    status = nx_packet_pool_create(&client_pool, "Client Packet Pool",
                                   PACKET_SIZE, client_packet_pool_area, PACKET_POOL_SIZE);
    if (status != NX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_ERROR " Failed to create packet pool: 0x%02X\r\n", status);
        return;
    }

    status = nx_ip_create(&client_ip, "Client IP", CLIENT_IP_ADDRESS_VAL,
                          NETWORK_MASK_VAL, &client_pool, nx_driver_imx,
                          client_ip_stack, DEMO_STACK_SIZE, 1);
    if (status != NX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_ERROR " Failed to create IP instance: 0x%02X\r\n", status);
        return;
    }

    nx_ip_gateway_address_set(&client_ip, GATEWAY_ADDRESS_VAL);
    nx_arp_enable(&client_ip, (VOID *)client_arp_cache, ARP_CACHE_SIZE);
    nx_icmp_enable(&client_ip);
    nx_tcp_enable(&client_ip);

    status = tx_thread_create(&client_test_thread, "Client Test Thread",
                              client_test_thread_entry, 0,
                              client_test_stack, DEMO_STACK_SIZE,
                              10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_ERROR " Failed to create test thread: 0x%02X\r\n", status);
    }
}

static UINT send_and_receive(NX_TCP_SOCKET *socket, const char *cmd, char *rx_buf, size_t rx_buf_size, ULONG timeout)
{
    NX_PACKET *tx_packet = NX_NULL;
    NX_PACKET *rx_packet = NX_NULL;
    UINT status;

    status = nx_packet_allocate(&client_pool, &tx_packet, NX_TCP_PACKET, TX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        return status;
    }

    status = nx_packet_data_append(tx_packet, (VOID *)cmd, strlen(cmd), &client_pool, TX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        nx_packet_release(tx_packet);
        return status;
    }

    status = nx_tcp_socket_send(socket, tx_packet, TX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        nx_packet_release(tx_packet);
        return status;
    }

    status = nx_tcp_socket_receive(socket, &rx_packet, timeout);
    if (status == NX_SUCCESS && rx_packet != NX_NULL)
    {
        ULONG bytes_copied = 0;

        /* Bounded extract: a response longer than the buffer is truncated. */
        nx_packet_data_extract_offset(rx_packet, 0, rx_buf,
                                      (ULONG)(rx_buf_size - 1), &bytes_copied);
        rx_buf[bytes_copied] = '\0';
        nx_packet_release(rx_packet);
    }
    return status;
}

static void client_test_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    UINT status;
    ULONG actual_status;
    int all_passed = 1;
    char buffer[256];

    printf(TAG_CLIENT " " MSG_INFO " Bringing Ethernet Link UP...\r\n");
    status = nx_ip_driver_direct_command(&client_ip, NX_LINK_ENABLE, &actual_status);
    if (status == NX_SUCCESS || status == NX_ALREADY_ENABLED)
    {
        printf(TAG_CLIENT " " MSG_SUCCESS " Ethernet link is UP!\r\n");
    }

    printf(TAG_CLIENT " " MSG_INFO " Waiting for network convergence...\r\n");
    tx_thread_sleep(150);

    printf("\r\n" ANSI_BOLD ANSI_CYAN "==================================================\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN " Starting Hardware TRNG & Console Verification Suite\r\n" ANSI_RESET);
    printf(ANSI_CYAN " Target Server: 192.168.0.100 (Port %d)\r\n" ANSI_RESET, CONSOLE_SERVER_PORT);
    printf(ANSI_BOLD ANSI_CYAN "==================================================\r\n\r\n" ANSI_RESET);

    /* Test 1: ICMP Ping */
    printf(TAG_CLIENT " [Test 1/6] Testing ICMP Ping to 192.168.0.100...\r\n");
    NX_PACKET *ping_resp = NX_NULL;
    status = nx_icmp_ping(&client_ip, SERVER_IP_ADDRESS, "TRNG_Ping", 9, &ping_resp, 200);
    if (status == NX_SUCCESS && ping_resp != NX_NULL)
    {
        printf(TAG_CLIENT " " MSG_SUCCESS " ICMP Ping successful! Response from 192.168.0.100\r\n");
        nx_packet_release(ping_resp);
    }
    else
    {
        printf(TAG_CLIENT " " MSG_ERROR " ICMP Ping failed: 0x%02X\r\n", status);
        all_passed = 0;
    }

    tx_thread_sleep(30);

    /* Test 2: Connect to TCP Port 23 */
    printf("\r\n" TAG_CLIENT " [Test 2/6] Connecting to TRNG Console Server on port %d...\r\n", CONSOLE_SERVER_PORT);
    NX_TCP_SOCKET client_socket;
    status = nx_tcp_socket_create(&client_ip, &client_socket, "Client Shell Socket",
                                  NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE,
                                  512, NX_NULL, NX_NULL);
    if (status != NX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_ERROR " Failed to create TCP socket: 0x%02X\r\n", status);
        return;
    }

    nx_tcp_client_socket_bind(&client_socket, NX_ANY_PORT, TX_WAIT_FOREVER);
    status = nx_tcp_client_socket_connect(&client_socket, SERVER_IP_ADDRESS, CONSOLE_SERVER_PORT, 200);
    if (status == NX_SUCCESS)
    {
        printf(TAG_CLIENT " " MSG_SUCCESS " TCP Connected! Receiving greeting banner...\r\n");

        /* Receive welcome banner */
        NX_PACKET *banner_packet = NX_NULL;
        if (nx_tcp_socket_receive(&client_socket, &banner_packet, 100) == NX_SUCCESS)
        {
            nx_packet_release(banner_packet);
        }

        /* Test 3: Query Hardware TRNG Entropy */
        printf("\r\n" TAG_CLIENT " [Test 3/6] Querying on-chip TRNG entropy ('trng')...\r\n");
        memset(buffer, 0, sizeof(buffer));
        status = send_and_receive(&client_socket, "trng\r\n", buffer, sizeof(buffer), 200);

        char *entropy_str = (status == NX_SUCCESS) ? strstr(buffer, "[TRNG] Hardware Entropy:") : NX_NULL;
        if (entropy_str != NX_NULL)
        {
            const char *vals_str = entropy_str + strlen("[TRNG] Hardware Entropy:");
            unsigned long w1 = 0, w2 = 0, w3 = 0, w4 = 0;
            int parsed = sscanf(vals_str, "%lx %lx %lx %lx", &w1, &w2, &w3, &w4);

            if (parsed != 4)
            {
                printf(TAG_CLIENT " " MSG_ERROR " Assertion failed: Expected 4 entropy words, parsed %d\r\n", parsed);
                all_passed = 0;
            }
            else if (w1 == 0 && w2 == 0 && w3 == 0 && w4 == 0)
            {
                printf(TAG_CLIENT " " MSG_ERROR " Assertion failed: All 4 entropy words are zero (0x00000000)\r\n");
                all_passed = 0;
            }
            else if (w1 == w2 || w1 == w3 || w1 == w4 || w2 == w3 || w2 == w4 || w3 == w4)
            {
                printf(TAG_CLIENT " " MSG_ERROR " Assertion failed: Entropy words are not distinct (0x%08lX 0x%08lX 0x%08lX 0x%08lX)\r\n",
                       w1, w2, w3, w4);
                all_passed = 0;
            }
            else
            {
                printf(TAG_CLIENT " " MSG_SUCCESS " Hardware TRNG Entropy Received:\r\n  %s", buffer);
                printf(TAG_CLIENT " " MSG_SUCCESS " Entropy words verified: 4 words parsed, non-zero, all mutually distinct.\r\n");

                /* Renode's PRNG is deterministic, so under --seed 12345 these four
                   words are a known sequence. Real silicon will not match them. */
                const unsigned long seed_12345_w1 = 0x69D43FF3UL;
                const unsigned long seed_12345_w2 = 0x54E900EEUL;
                const unsigned long seed_12345_w3 = 0x2514F462UL;
                const unsigned long seed_12345_w4 = 0x39F5B5D8UL;

                if (w1 == seed_12345_w1 && w2 == seed_12345_w2 && w3 == seed_12345_w3 && w4 == seed_12345_w4)
                {
                    printf(TAG_CLIENT " " MSG_SUCCESS " Deterministic seed (12345) PRNG sequence verified exactly!\r\n");
                }
                else
                {
                    printf(TAG_CLIENT " Entropy does not match the seed-12345 reference sequence, which is expected on hardware or under another seed.\r\n");
                }
            }
        }
        else
        {
            printf(TAG_CLIENT " " MSG_ERROR " TRNG query failed (status: 0x%02X, response: '%s')\r\n", status, buffer);
            all_passed = 0;
        }

        /* Test 4: Remote LED Control */
        printf("\r\n" TAG_CLIENT " [Test 4/6] Testing Remote LED Control ('led toggle')...\r\n");
        memset(buffer, 0, sizeof(buffer));
        status = send_and_receive(&client_socket, "led toggle\r\n", buffer, sizeof(buffer), 200);
        if (status == NX_SUCCESS && strstr(buffer, "[LED] State: TOGGLED"))
        {
            printf(TAG_CLIENT " " MSG_SUCCESS " Remote LED toggle acknowledged by server!\r\n");
        }
        else
        {
            printf(TAG_CLIENT " " MSG_ERROR " LED control failed (status: 0x%02X)\r\n", status);
            all_passed = 0;
        }

        /* Test 5: Target Info Query */
        printf("\r\n" TAG_CLIENT " [Test 5/6] Querying processor and RTOS status ('info')...\r\n");
        memset(buffer, 0, sizeof(buffer));
        status = send_and_receive(&client_socket, "info\r\n", buffer, sizeof(buffer), 200);
        if (status == NX_SUCCESS && strstr(buffer, "MIMXRT1064-EVK"))
        {
            printf(TAG_CLIENT " " MSG_SUCCESS " Processor & ThreadX status verified:\r\n  %s", buffer);
        }
        else
        {
            printf(TAG_CLIENT " " MSG_ERROR " Info query failed (status: 0x%02X)\r\n", status);
            all_passed = 0;
        }

        /* Test 6: an over-long line is refused and the session survives it */
        printf("\r\n" TAG_CLIENT " [Test 6/6] Testing over-long command line rejection...\r\n");
        {
            char long_cmd[200];

            memset(long_cmd, 'A', sizeof(long_cmd) - 3);
            long_cmd[sizeof(long_cmd) - 3] = '\r';
            long_cmd[sizeof(long_cmd) - 2] = '\n';
            long_cmd[sizeof(long_cmd) - 1] = '\0';

            memset(buffer, 0, sizeof(buffer));
            status = send_and_receive(&client_socket, long_cmd, buffer, sizeof(buffer), 200);

            if (status == NX_SUCCESS && strstr(buffer, "Command line too long"))
            {
                memset(buffer, 0, sizeof(buffer));
                status = send_and_receive(&client_socket, "ping\r\n", buffer, sizeof(buffer), 200);
                if (status == NX_SUCCESS && strstr(buffer, "[PONG]"))
                {
                    printf(TAG_CLIENT " " MSG_SUCCESS " Over-long line refused, session still responsive.\r\n");
                }
                else
                {
                    printf(TAG_CLIENT " " MSG_ERROR " Session unresponsive after over-long line (status: 0x%02X)\r\n", status);
                    all_passed = 0;
                }
            }
            else
            {
                printf(TAG_CLIENT " " MSG_ERROR " Over-long line was not refused (status: 0x%02X, response: '%s')\r\n",
                       status, buffer);
                all_passed = 0;
            }
        }

        /* Graceful disconnect */
        send_and_receive(&client_socket, "quit\r\n", buffer, sizeof(buffer), 50);
        nx_tcp_socket_disconnect(&client_socket, 10);
    }
    else
    {
        printf(TAG_CLIENT " " MSG_ERROR " Failed to connect to server: 0x%02X\r\n", status);
        all_passed = 0;
    }

    nx_tcp_client_socket_unbind(&client_socket);
    nx_tcp_socket_delete(&client_socket);

    printf("\r\n==================================================\r\n");
    if (all_passed)
    {
        printf(ANSI_BOLD ANSI_GREEN " [VERIFICATION SUCCESS] ALL TRNG & CONSOLE TESTS PASSED!\r\n" ANSI_RESET);
    }
    else
    {
        printf(ANSI_BOLD ANSI_RED " [VERIFICATION FAILED] One or more tests failed.\r\n" ANSI_RESET);
    }
    printf("==================================================\r\n\r\n");

    while (1)
    {
        tx_thread_sleep(100);
    }
}
