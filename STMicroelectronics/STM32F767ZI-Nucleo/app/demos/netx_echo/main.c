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

#include "board_init.h"
#include "tx_api.h"
#include "nx_api.h"
#include "nxd_dhcp_client.h"
#include <stdio.h>

#define DEMO_STACK_SIZE         2048
#define PACKET_SIZE             1536
#define PACKET_POOL_SIZE        (PACKET_SIZE * 20)
#define ECHO_SERVER_PORT        7

#define ARP_CACHE_SIZE          1024

TX_THREAD               dhcp_thread;
uint8_t                 dhcp_thread_stack[DEMO_STACK_SIZE];

TX_THREAD               tcp_echo_thread;
uint8_t                 tcp_echo_stack[DEMO_STACK_SIZE];

uint8_t                 ip_thread_stack[DEMO_STACK_SIZE];
uint8_t                 arp_cache_area[ARP_CACHE_SIZE];

NX_PACKET_POOL          pool_0;
NX_IP                   ip_0;
NX_DHCP                 dhcp_client;

/* Place the NetX Duo packet pool in the .nx_data section (uncacheable memory) */
__attribute__((section(".NetXPoolSection"))) uint8_t packet_pool_area[PACKET_POOL_SIZE];

extern VOID nx_stm32_eth_driver(NX_IP_DRIVER *driver_req_ptr);

void dhcp_thread_entry(ULONG thread_input);
void tcp_echo_thread_entry(ULONG thread_input);

int main(void)
{
    board_init();
    printf("\r\n==========================================\r\n");
    printf("NetX Echo Demo Booted!\r\n");
    printf("==========================================\r\n\r\n");

    /* Initialize the Ethernet hardware, MAC, and wait for link to be established */
    board_ethernet_init();

    tx_kernel_enter();
    return 0;
}

void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    nx_system_initialize();

    /* Create the packet pool in our uncacheable section */
    if (nx_packet_pool_create(&pool_0, "NetX Main Packet Pool",
                              PACKET_SIZE, packet_pool_area, PACKET_POOL_SIZE) != NX_SUCCESS) {
        printf("[NetX] Failed to create packet pool.\r\n");
        return;
    }
    printf("[NetX] Packet pool created\r\n");

    /* Create the IP instance using the STM32 Ethernet driver */
    if (nx_ip_create(&ip_0, "NetX IP Instance 0", IP_ADDRESS(0, 0, 0, 0),
                     0xFFFFFF00UL, &pool_0, nx_stm32_eth_driver,
                     ip_thread_stack, DEMO_STACK_SIZE, 1) != NX_SUCCESS) {
        printf("[NetX] Failed to create IP instance.\r\n");
        return;
    }
    printf("[NetX] IP instance created\r\n");

    /* Enable ARP, TCP, UDP, and ICMP */
    printf("[NetX] Enabling ARP...\r\n");
    nx_arp_enable(&ip_0, (VOID *)arp_cache_area, ARP_CACHE_SIZE);
    printf("[NetX] Enabling TCP...\r\n");
    nx_tcp_enable(&ip_0);
    printf("[NetX] Enabling UDP...\r\n");
    nx_udp_enable(&ip_0);
    printf("[NetX] Enabling ICMP...\r\n");
    nx_icmp_enable(&ip_0);

    /* Start DHCP Thread */
    printf("[NetX] Creating DHCP Thread...\r\n");
    tx_thread_create(&dhcp_thread, "DHCP Thread", dhcp_thread_entry, 0,
                     dhcp_thread_stack, DEMO_STACK_SIZE, 2, 2, TX_NO_TIME_SLICE, TX_AUTO_START);
    printf("[NetX] tx_application_define completed\r\n");
}

void dhcp_thread_entry(ULONG thread_input)
{
    ULONG ip_address;
    ULONG network_mask;
    ULONG actual_status;
    UINT prev_link_up = NX_FALSE;
    UINT tcp_server_started = NX_FALSE;
    extern int32_t nx_eth_phy_get_link_state(void);

    /* Create the DHCP client instance once */
    nx_dhcp_create(&dhcp_client, &ip_0, "DHCP Client");

    printf("[NetX] Network Monitor Thread Started.\r\n");

    while (1)
    {
        /* Poll the physical link state */
        int32_t phy_state = nx_eth_phy_get_link_state();
        UINT curr_link_up = (phy_state > 1) ? NX_TRUE : NX_FALSE;

        if (curr_link_up != prev_link_up)
        {
            if (curr_link_up)
            {
                printf("[NetX] Ethernet cable connected! Enabling link...\r\n");
                UINT status = nx_ip_driver_direct_command(&ip_0, NX_LINK_ENABLE, &actual_status);
                if (status == NX_SUCCESS || status == NX_ALREADY_ENABLED)
                {
                    printf("[NetX] Ethernet link is UP! Starting DHCP client...\r\n");
                    nx_dhcp_start(&dhcp_client);

                    /* Wait up to 10 seconds for IP address resolution */
                    if (nx_ip_status_check(&ip_0, NX_IP_ADDRESS_RESOLVED, &ip_address, 1000) == NX_SUCCESS)
                    {
                        nx_ip_address_get(&ip_0, &ip_address, &network_mask);
                        printf("[NetX] DHCP Success!\r\n");
                        printf("[NetX] IP Address : %lu.%lu.%lu.%lu\r\n", 
                               (ip_address >> 24) & 0xFF, (ip_address >> 16) & 0xFF, 
                               (ip_address >> 8) & 0xFF, ip_address & 0xFF);
                        printf("[NetX] Subnet Mask: %lu.%lu.%lu.%lu\r\n", 
                               (network_mask >> 24) & 0xFF, (network_mask >> 16) & 0xFF, 
                               (network_mask >> 8) & 0xFF, network_mask & 0xFF);
                    }
                    else
                    {
                        printf("[NetX] DHCP Timeout! Using static IP 192.168.0.100\r\n");
                        nx_dhcp_stop(&dhcp_client);
                        nx_ip_address_set(&ip_0, IP_ADDRESS(192, 168, 0, 100), 0xFFFFFF00UL);
                    }

                    /* Start the TCP Echo Server thread if not already started */
                    if (!tcp_server_started)
                    {
                        tx_thread_create(&tcp_echo_thread, "TCP Echo Thread", tcp_echo_thread_entry, 0,
                                         tcp_echo_stack, DEMO_STACK_SIZE, 3, 3, TX_NO_TIME_SLICE, TX_AUTO_START);
                        tcp_server_started = NX_TRUE;
                    }
                }
                else
                {
                    printf("[NetX] Failed to enable driver link: 0x%02x\r\n", status);
                }
            }
            else
            {
                printf("[NetX] Ethernet cable disconnected! Disabling link...\r\n");
                nx_dhcp_stop(&dhcp_client);
                nx_ip_driver_direct_command(&ip_0, NX_LINK_DISABLE, &actual_status);
                /* Reset IP address to 0.0.0.0 */
                nx_ip_address_set(&ip_0, IP_ADDRESS(0, 0, 0, 0), 0xFFFFFF00UL);
            }
            prev_link_up = curr_link_up;
        }

        tx_thread_sleep(100); /* Poll every 1 second (100 ticks) */
    }
}

void tcp_echo_thread_entry(ULONG thread_input)
{
    NX_TCP_SOCKET echo_socket;
    NX_PACKET *packet_ptr;

    nx_tcp_socket_create(&ip_0, &echo_socket, "Echo Socket", NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 512, NX_NULL, NX_NULL);

    printf("[Echo] TCP Echo Server listening on port %d\r\n", ECHO_SERVER_PORT);

    while (1) {
        if (nx_tcp_server_socket_listen(&ip_0, ECHO_SERVER_PORT, &echo_socket, 5, NX_NULL) != NX_SUCCESS) {
            nx_tcp_server_socket_unlisten(&ip_0, ECHO_SERVER_PORT);
            continue;
        }

        if (nx_tcp_server_socket_accept(&echo_socket, NX_WAIT_FOREVER) == NX_SUCCESS) {
            printf("[Echo] Client connected.\r\n");
            
            while (nx_tcp_socket_receive(&echo_socket, &packet_ptr, NX_WAIT_FOREVER) == NX_SUCCESS) {
                /* Echo the packet back */
                nx_tcp_socket_send(&echo_socket, packet_ptr, NX_WAIT_FOREVER);
            }
            
            printf("[Echo] Client disconnected.\r\n");
            nx_tcp_socket_disconnect(&echo_socket, NX_WAIT_FOREVER);
            nx_tcp_server_socket_unaccept(&echo_socket);
        }
        nx_tcp_server_socket_unlisten(&ip_0, ECHO_SERVER_PORT);
    }
}
