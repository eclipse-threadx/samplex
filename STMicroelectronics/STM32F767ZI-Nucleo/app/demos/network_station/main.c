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

#include "tx_api.h"
#include "fx_api.h"
#include "nx_api.h"
#include "nxd_dhcp_client.h"
#include "nxd_dns.h"
#include "nxd_mqtt_client.h"
#include "board_init.h"
#include "sensor.h"
#include "mqtt_config.h"
#include <stdio.h>
#include <string.h>
#include "ansi_colors.h"

#define DEMO_STACK_SIZE         2048
#define DEMO_BYTE_POOL_SIZE     65536  /* Increased byte pool size to accommodate NetX Duo and ThreadX stacks */
#define SENSOR_QUEUE_MAX_MSGS   10
#define SENSOR_MSG_WORDS        3      /* 12 bytes = 3 ULONGs */
#define RAM_DISK_SIZE           32768  /* 32 KB RAM Disk */
#define RAM_DISK_SECTOR_SIZE    512

/* NetX Duo Configuration Constants */
#define PACKET_SIZE             1536
#define PACKET_POOL_SIZE        (PACKET_SIZE * 20)
#define ARP_CACHE_SIZE          1024

/* ThreadX objects */
TX_THREAD               sensor_thread;
TX_THREAD               logger_thread;
TX_THREAD               network_thread;
TX_THREAD               status_thread;
TX_THREAD               dhcp_thread;

TX_QUEUE                sensor_queue;
TX_QUEUE                network_queue;
TX_BYTE_POOL            byte_pool_sensor;

/* FileX objects */
FX_MEDIA                ram_disk;
FX_FILE                 log_file;

/* NetX Duo objects */
NX_PACKET_POOL          pool_0;
NX_IP                   ip_0;
NX_DHCP                 dhcp_client;
NX_DNS                  dns_client;
NXD_MQTT_CLIENT         mqtt_client;

/* Connection States shared with Status Thread */
static volatile UINT ip_resolved = NX_FALSE;
static volatile UINT mqtt_connected = NX_FALSE;
static char mqtt_topic[64];

/* Place the NetX Duo packet pool in the .nx_data section (uncacheable memory) */
__attribute__((section(".NetXPoolSection"))) uint8_t packet_pool_area[PACKET_POOL_SIZE];
static uint8_t arp_cache_area[ARP_CACHE_SIZE];

/* Memory area for byte pool */
static uint8_t pool_memory[DEMO_BYTE_POOL_SIZE];

/* Statically allocated memory for RAM disk and sector cache */
static uint8_t ram_disk_memory[RAM_DISK_SIZE];
static uint8_t cache_buffer[RAM_DISK_SECTOR_SIZE];

/* External FileX and NetX driver prototypes */
VOID _fx_ram_driver(FX_MEDIA *media_ptr);
extern VOID nx_stm32_eth_driver(NX_IP_DRIVER *driver_req_ptr);

/* Thread entry prototypes */
static void sensor_thread_entry(ULONG thread_input);
static void logger_thread_entry(ULONG thread_input);
static void network_thread_entry(ULONG thread_input);
static void status_thread_entry(ULONG thread_input);
static void dhcp_thread_entry(ULONG thread_input);

int main(void)
{
    /* Initialize Board Support Package (BSP) - clock, GPIO, UART, I2C, etc. */
    board_init();

    printf(ANSI_BOLD ANSI_CYAN "\r\n==================================================\r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN "  Network Environmental Station Demo   \r\n" ANSI_RESET);
    printf(ANSI_BOLD ANSI_CYAN "==================================================\r\n" ANSI_RESET);

    /* Initialize the Ethernet hardware, MAC, and wait for link to be established */
    board_ethernet_init();

    /* Enter the ThreadX kernel */
    tx_kernel_enter();

    return 0;
}

void tx_application_define(void *first_unused_memory)
{
    CHAR *pointer;
    (void)first_unused_memory;

    /* Create a byte memory pool from which to allocate the thread stacks and queues */
    if (tx_byte_pool_create(&byte_pool_sensor, "sensor_pool", pool_memory, DEMO_BYTE_POOL_SIZE) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Allocate and create sensor message queue */
    if (tx_byte_allocate(&byte_pool_sensor, (VOID **) &pointer, 
                         SENSOR_QUEUE_MAX_MSGS * SENSOR_MSG_WORDS * sizeof(ULONG), TX_NO_WAIT) != TX_SUCCESS)
    {
        Error_Handler();
    }
    if (tx_queue_create(&sensor_queue, "sensor_queue", SENSOR_MSG_WORDS, pointer, 
                        SENSOR_QUEUE_MAX_MSGS * SENSOR_MSG_WORDS * sizeof(ULONG)) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Allocate and create network message queue */
    if (tx_byte_allocate(&byte_pool_sensor, (VOID **) &pointer, 
                         SENSOR_QUEUE_MAX_MSGS * SENSOR_MSG_WORDS * sizeof(ULONG), TX_NO_WAIT) != TX_SUCCESS)
    {
        Error_Handler();
    }
    if (tx_queue_create(&network_queue, "network_queue", SENSOR_MSG_WORDS, pointer, 
                        SENSOR_QUEUE_MAX_MSGS * SENSOR_MSG_WORDS * sizeof(ULONG)) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Allocate the stack for the sensor thread */
    if (tx_byte_allocate(&byte_pool_sensor, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
        Error_Handler();
    }
    /* Create the sensor thread (Priority 3) */
    if (tx_thread_create(&sensor_thread, "sensor_thread", sensor_thread_entry, 0,  
                         pointer, DEMO_STACK_SIZE, 
                         3, 3, TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Allocate the stack for the logger thread */
    if (tx_byte_allocate(&byte_pool_sensor, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
        Error_Handler();
    }
    /* Create the logger thread (Priority 5) */
    if (tx_thread_create(&logger_thread, "logger_thread", logger_thread_entry, 0,  
                         pointer, DEMO_STACK_SIZE, 
                         5, 5, TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Allocate the stack for the network telemetry thread */
    if (tx_byte_allocate(&byte_pool_sensor, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
        Error_Handler();
    }
    /* Create the network thread (Priority 4) */
    if (tx_thread_create(&network_thread, "network_thread", network_thread_entry, 0,  
                         pointer, DEMO_STACK_SIZE, 
                         4, 4, TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Allocate the stack for the status monitor thread */
    if (tx_byte_allocate(&byte_pool_sensor, (VOID **) &pointer, 1024, TX_NO_WAIT) != TX_SUCCESS)
    {
        Error_Handler();
    }
    /* Create the status thread (Priority 6) */
    if (tx_thread_create(&status_thread, "status_thread", status_thread_entry, 0,  
                         pointer, 1024, 
                         6, 6, TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Initialize NetX Duo System */
    nx_system_initialize();

    /* Create the packet pool in our uncacheable section */
    if (nx_packet_pool_create(&pool_0, "NetX Main Packet Pool",
                              PACKET_SIZE, packet_pool_area, PACKET_POOL_SIZE) != NX_SUCCESS) {
        printf(ANSI_BLUE "[NetX] " ANSI_RED "Failed to create packet pool.\r\n" ANSI_RESET);
        Error_Handler();
    }

    /* Allocate the stack for NetX IP thread */
    if (tx_byte_allocate(&byte_pool_sensor, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Create the IP instance using the STM32 Ethernet driver */
    if (nx_ip_create(&ip_0, "NetX IP Instance 0", IP_ADDRESS(0, 0, 0, 0),
                     0xFFFFFF00UL, &pool_0, nx_stm32_eth_driver,
                     pointer, DEMO_STACK_SIZE, 1) != NX_SUCCESS) {
        printf(ANSI_BLUE "[NetX] " ANSI_RED "Failed to create IP instance.\r\n" ANSI_RESET);
        Error_Handler();
    }

    /* Enable ARP, TCP, UDP, and ICMP */
    nx_arp_enable(&ip_0, (VOID *)arp_cache_area, ARP_CACHE_SIZE);
    nx_tcp_enable(&ip_0);
    nx_udp_enable(&ip_0);
    nx_icmp_enable(&ip_0);

    /* Allocate the stack for the DHCP client thread */
    if (tx_byte_allocate(&byte_pool_sensor, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Create DHCP/Link Monitor Thread (Priority 2) */
    if (tx_thread_create(&dhcp_thread, "DHCP/Link Thread", dhcp_thread_entry, 0,
                         pointer, DEMO_STACK_SIZE, 2, 2, TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
    {
        Error_Handler();
    }

    printf(ANSI_BOLD ANSI_WHITE "[System] " ANSI_GREEN "ThreadX, FileX, and NetX objects initialized successfully.\r\n" ANSI_RESET);
}

static void sensor_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    float temp = 0.0f;
    float hum = 0.0f;
    sensor_msg_t msg;

    /* Initialize sensor hardware (or fallback to mock) */
    if (sensor_init() != SENSOR_OK)
    {
        printf(ANSI_CYAN "[Sensor Thread] " ANSI_RED "Critical Error: Sensor initialization failed!\r\n" ANSI_RESET);
        Error_Handler();
    }

    printf(ANSI_CYAN "[Sensor Thread] " ANSI_WHITE "Started polling loop (every 2 seconds).\r\n" ANSI_RESET);

    while (1)
    {
        /* Read sensor data */
        if (sensor_read(&temp, &hum) == SENSOR_OK)
        {
            /* Pack the message */
            msg.temperature = temp;
            msg.humidity = hum;
            msg.timestamp = HAL_GetTick();

            /* Send the message to the queue */
            if (tx_queue_send(&sensor_queue, &msg, TX_NO_WAIT) == TX_SUCCESS)
            {
                float temp_abs = (temp < 0.0f) ? -temp : temp;
                int temp_dec = (int)temp_abs;
                int temp_frac = (int)((temp_abs - (float)temp_dec) * 100.0f);
                
                int hum_dec = (int)hum;
                int hum_frac = (int)((hum - (float)hum_dec) * 100.0f);

                printf(ANSI_CYAN "[Sensor Thread] " ANSI_WHITE "Temp: %s%d.%02d C, Hum: %d.%02d %%, Time: %lu ms (Sent to Queue)\r\n" ANSI_RESET, 
                       (temp < 0.0f) ? "-" : "", temp_dec, temp_frac, hum_dec, hum_frac, (unsigned long)msg.timestamp);
            }
            else
            {
                printf(ANSI_CYAN "[Sensor Thread] " ANSI_YELLOW "WARNING: Queue is full, message dropped.\r\n" ANSI_RESET);
            }
        }
        else
        {
            printf(ANSI_CYAN "[Sensor Thread] " ANSI_RED "Error: Failed to read sensor data.\r\n" ANSI_RESET);
        }

        /* Sleep for 2 seconds (assuming 100 ticks per second) */
        tx_thread_sleep(2 * TX_TIMER_TICKS_PER_SECOND);
    }
}

static void logger_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    UINT status;
    sensor_msg_t msg;
    char write_buffer[64];

    printf(ANSI_MAGENTA "[Logger Thread] " ANSI_WHITE "Starting FileX RAM Disk Logger...\r\n" ANSI_RESET);

    /* Initialize FileX system */
    fx_system_initialize();

    /* Format the RAM disk. This is required before opening it. */
    status = fx_media_format(&ram_disk, 
                             _fx_ram_driver, 
                             ram_disk_memory, 
                             cache_buffer, 
                             sizeof(cache_buffer), 
                             "RAM DISK", 
                             1, 
                             32, 
                             0, 
                             RAM_DISK_SIZE / RAM_DISK_SECTOR_SIZE, 
                             RAM_DISK_SECTOR_SIZE, 
                             1, 
                             1, 
                             1);
    if (status != FX_SUCCESS)
    {
        printf(ANSI_MAGENTA "[Logger Thread] " ANSI_RED "Error: RAM Disk format failed! (0x%02X)\r\n" ANSI_RESET, status);
        Error_Handler();
    }

    /* Open the RAM Disk */
    status = fx_media_open(&ram_disk, "RAM DISK", _fx_ram_driver, ram_disk_memory, 
                           cache_buffer, sizeof(cache_buffer));
    if (status != FX_SUCCESS)
    {
        printf(ANSI_MAGENTA "[Logger Thread] " ANSI_RED "Error: RAM Disk open failed! (0x%02X)\r\n" ANSI_RESET, status);
        Error_Handler();
    }

    /* Create the log file */
    status = fx_file_create(&ram_disk, "sensor_log.txt");
    if (status != FX_SUCCESS && status != FX_ALREADY_CREATED)
    {
        printf(ANSI_MAGENTA "[Logger Thread] " ANSI_RED "Error: Failed to create log file! (0x%02X)\r\n" ANSI_RESET, status);
        Error_Handler();
    }

    /* Open the log file for writing */
    status = fx_file_open(&ram_disk, &log_file, "sensor_log.txt", FX_OPEN_FOR_WRITE);
    if (status != FX_SUCCESS)
    {
        printf(ANSI_MAGENTA "[Logger Thread] " ANSI_RED "Error: Failed to open log file! (0x%02X)\r\n" ANSI_RESET, status);
        Error_Handler();
    }

    /* Move the write pointer to the end of the file to append new data */
    fx_file_seek(&log_file, log_file.fx_file_current_file_size);

    printf(ANSI_MAGENTA "[Logger Thread] " ANSI_GREEN "RAM Disk initialized. Logging 'sensor_log.txt'...\r\n" ANSI_RESET);

    while (1)
    {
        /* Retrieve the next sensor reading from the queue (blocks until a message is available) */
        if (tx_queue_receive(&sensor_queue, &msg, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            /* Format the sensor reading as a CSV line: timestamp,temp,hum */
            float temp_abs = (msg.temperature < 0.0f) ? -msg.temperature : msg.temperature;
            int temp_dec = (int)temp_abs;
            int temp_frac = (int)((temp_abs - (float)temp_dec) * 100.0f);
            
            int hum_dec = (int)msg.humidity;
            int hum_frac = (int)((msg.humidity - (float)hum_dec) * 100.0f);

            int len = snprintf(write_buffer, sizeof(write_buffer), "%lu,%s%d.%02d,%d.%02d\n",
                               (unsigned long)msg.timestamp,
                               (msg.temperature < 0.0f) ? "-" : "", temp_dec, temp_frac,
                               hum_dec, hum_frac);

            if (len > 0)
            {
                /* Write the CSV line to the file */
                status = fx_file_write(&log_file, write_buffer, (ULONG)len);
                if (status == FX_SUCCESS)
                {
                    /* Flush the media to commit changes to the RAM Disk immediately */
                    fx_media_flush(&ram_disk);
                    
                    printf(ANSI_MAGENTA "[Logger Thread] " ANSI_WHITE "Logged to RAM Disk: %lu,%s%d.%02d,%d.%02d\r\n" ANSI_RESET,
                           (unsigned long)msg.timestamp,
                           (msg.temperature < 0.0f) ? "-" : "", temp_dec, temp_frac,
                           hum_dec, hum_frac);
                }
                else
                {
                    printf(ANSI_MAGENTA "[Logger Thread] " ANSI_RED "Error: Failed to write to log file! (0x%02X)\r\n" ANSI_RESET, status);
                }
            }

            /* Forward the sensor reading to the network telemetry queue (Evict Oldest policy) */
            if (tx_queue_send(&network_queue, &msg, TX_NO_WAIT) != TX_SUCCESS)
            {
                sensor_msg_t discarded_msg;
                /* Evict the oldest item at the front of the queue to make room */
                if (tx_queue_receive(&network_queue, &discarded_msg, TX_NO_WAIT) == TX_SUCCESS)
                {
                    printf(ANSI_MAGENTA "[Logger Thread] " ANSI_YELLOW "WARNING: Network queue full. Evicting oldest message (Time: %lu ms).\r\n" ANSI_RESET, 
                           (unsigned long)discarded_msg.timestamp);
                }
                
                /* Try sending the new message again */
                tx_queue_send(&network_queue, &msg, TX_NO_WAIT);
            }
        }
    }
}

static void dhcp_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    ULONG ip_address;
    ULONG network_mask;
    ULONG actual_status;
    UINT prev_link_up = NX_FALSE;
    extern int32_t nx_eth_phy_get_link_state(void);

    /* Create the DHCP client instance once */
    nx_dhcp_create(&dhcp_client, &ip_0, "DHCP Client");

    printf(ANSI_BLUE "[NetX] " ANSI_WHITE "Network Monitor Thread Started.\r\n" ANSI_RESET);

    while (1)
    {
        /* Poll the physical link state */
        int32_t phy_state = nx_eth_phy_get_link_state();
        UINT curr_link_up = (phy_state > 1) ? NX_TRUE : NX_FALSE;

        if (curr_link_up != prev_link_up)
        {
            if (curr_link_up)
            {
                printf(ANSI_BLUE "[NetX] " ANSI_YELLOW "Ethernet cable connected! Enabling link...\r\n" ANSI_RESET);
                UINT status = nx_ip_driver_direct_command(&ip_0, NX_LINK_ENABLE, &actual_status);
                if (status == NX_SUCCESS || status == NX_ALREADY_ENABLED)
                {
                    printf(ANSI_BLUE "[NetX] " ANSI_GREEN "Ethernet link is UP! Starting DHCP client...\r\n" ANSI_RESET);
                    nx_dhcp_start(&dhcp_client);

                    /* Wait up to 10 seconds for IP address resolution */
                    if (nx_ip_status_check(&ip_0, NX_IP_ADDRESS_RESOLVED, &ip_address, 1000) == NX_SUCCESS)
                    {
                        nx_ip_address_get(&ip_0, &ip_address, &network_mask);
                        printf(ANSI_BLUE "[NetX] " ANSI_BOLD ANSI_GREEN "DHCP Success!\r\n" ANSI_RESET);
                        printf(ANSI_BLUE "[NetX] " ANSI_GREEN "IP Address : %lu.%lu.%lu.%lu\r\n" ANSI_RESET, 
                               (ip_address >> 24) & 0xFF, (ip_address >> 16) & 0xFF, 
                               (ip_address >> 8) & 0xFF, ip_address & 0xFF);
                        ip_resolved = NX_TRUE;
                    }
                    else
                    {
                        printf(ANSI_BLUE "[NetX] " ANSI_YELLOW "DHCP Timeout! Using static IP 192.168.0.100\r\n" ANSI_RESET);
                        nx_dhcp_stop(&dhcp_client);
                        nx_ip_address_set(&ip_0, IP_ADDRESS(192, 168, 0, 100), 0xFFFFFF00UL);
                        ip_resolved = NX_TRUE;
                    }
                }
                else
                {
                    printf(ANSI_BLUE "[NetX] " ANSI_RED "Failed to enable driver link: 0x%02x\r\n" ANSI_RESET, status);
                }
            }
            else
            {
                printf(ANSI_BLUE "[NetX] " ANSI_RED "Ethernet cable disconnected! Disabling link...\r\n" ANSI_RESET);
                ip_resolved = NX_FALSE;
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

static void network_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    UINT status;
    sensor_msg_t msg;
    char payload[128];
    NXD_ADDRESS broker_ip;
    VOID *mqtt_stack;
    char client_id[32];
    uint32_t uid_w0 = HAL_GetUIDw0();
    
    /* Format unique topic and client ID using board's hardware unique ID */
    snprintf(client_id, sizeof(client_id), "stm32f767_%08lX", (unsigned long)uid_w0);
    snprintf(mqtt_topic, sizeof(mqtt_topic), "samplex/env_station/%s/telemetry", client_id);

    printf(ANSI_BLUE "[Network Thread] " ANSI_WHITE "Starting Network Telemetry Thread...\r\n" ANSI_RESET);

    /* Allocate stack for internal MQTT thread from our byte memory pool */
    if (tx_byte_allocate(&byte_pool_sensor, (VOID **) &mqtt_stack, DEMO_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
        Error_Handler();
    }

    /* Create the NetX Duo MQTT Client Instance */
    status = nxd_mqtt_client_create(&mqtt_client, "Samplex Client", client_id, strlen(client_id),
                                    &ip_0, &pool_0, mqtt_stack, DEMO_STACK_SIZE, 4, NX_NULL, 0);
    if (status != NX_SUCCESS)
    {
        printf(ANSI_BLUE "[MQTT] " ANSI_RED "Error: Failed to create MQTT client (0x%02X)\r\n" ANSI_RESET, status);
        Error_Handler();
    }

    /* Reconnection/Publish state machine */
    while (1)
    {
        /* 1. Wait until network link is established and IP is resolved */
        while (!ip_resolved)
        {
            tx_thread_sleep(100);
        }

        /* 2. Initialize DNS Client if not already running */
        static UINT dns_initialized = NX_FALSE;
        if (!dns_initialized)
        {
            /* Fallback to Google DNS */
            ULONG dns_server = IP_ADDRESS(8, 8, 8, 8);
            UINT size = sizeof(dns_server);
            status = nx_dhcp_user_option_retrieve(&dhcp_client, NX_DHCP_OPTION_DNS_SVR, (UCHAR *)&dns_server, &size);

            if (status != NX_SUCCESS)
            {
                printf(ANSI_BLUE "[DNS] " ANSI_YELLOW "Warning: Failed to retrieve DHCP DNS option (0x%02X). Using fallback Google DNS.\r\n" ANSI_RESET, status);
            }
            
            if (nx_dns_create(&dns_client, &ip_0, (UCHAR *)"DNS Client") == NX_SUCCESS)
            {
                nx_dns_server_add(&dns_client, dns_server);
                dns_initialized = NX_TRUE;
                printf(ANSI_BLUE "[DNS] " ANSI_GREEN "DNS Client initialized. Server: %lu.%lu.%lu.%lu\r\n" ANSI_RESET,
                       (dns_server >> 24) & 0xFF, (dns_server >> 16) & 0xFF,
                       (dns_server >> 8) & 0xFF, dns_server & 0xFF);
            }
            else
            {
                printf(ANSI_BLUE "[DNS] " ANSI_RED "Error: Failed to create DNS client! Retrying...\r\n" ANSI_RESET);
                tx_thread_sleep(200);
                continue;
            }
        }

        /* 3. Resolve DNS name of the Mosquitto broker */
        broker_ip.nxd_ip_version = NX_IP_VERSION_V4;
        status = nxd_dns_host_by_name_get(&dns_client, (UCHAR *)MQTT_BROKER_HOST, &broker_ip, 500, NX_IP_VERSION_V4);
        if (status != NX_SUCCESS)
        {
            printf(ANSI_BLUE "[DNS] " ANSI_RED "Error: Failed to resolve broker '%s' (0x%02X). Retrying...\r\n" ANSI_RESET, MQTT_BROKER_HOST, status);
            tx_thread_sleep(500);
            continue;
        }

        printf(ANSI_BLUE "[DNS] " ANSI_GREEN "Resolved '%s' to %lu.%lu.%lu.%lu\r\n" ANSI_RESET, MQTT_BROKER_HOST,
               (broker_ip.nxd_ip_address.v4 >> 24) & 0xFF,
               (broker_ip.nxd_ip_address.v4 >> 16) & 0xFF,
               (broker_ip.nxd_ip_address.v4 >> 8) & 0xFF,
               broker_ip.nxd_ip_address.v4 & 0xFF);

        /* 4. Connect to Mosquitto Broker */
        printf(ANSI_BLUE "[MQTT] " ANSI_YELLOW "Connecting to broker %s:%d...\r\n" ANSI_RESET, MQTT_BROKER_HOST, MQTT_BROKER_PORT);
        status = nxd_mqtt_client_connect(&mqtt_client, &broker_ip, MQTT_BROKER_PORT,
                                         60, 1, 1000); /* 60s keepalive, clean session, 10s wait */
        if (status != NX_SUCCESS)
        {
            printf(ANSI_BLUE "[MQTT] " ANSI_RED "Error: Connection failed (0x%02X). Retrying...\r\n" ANSI_RESET, status);
            tx_thread_sleep(500);
            continue;
        }

        printf(ANSI_BLUE "[MQTT] " ANSI_BOLD ANSI_GREEN "Connected successfully to Mosquitto broker!\r\n" ANSI_RESET);
        printf(ANSI_BLUE "[MQTT] " ANSI_WHITE "Publishing to topic: %s\r\n" ANSI_RESET, mqtt_topic);
        mqtt_connected = NX_TRUE;

        /* 5. Publish Telemetry Loop */
        while (ip_resolved && mqtt_connected)
        {
            /* Retrieve messages from the network forwarding queue (blocks until available) */
            if (tx_queue_receive(&network_queue, &msg, 500) == TX_SUCCESS)
            {
                /* Check if the link was lost while waiting for the message */
                if (!ip_resolved || !mqtt_connected)
                {
                    /* Put the message back so its not lost */
                    UINT queue_status = tx_queue_front_send(&network_queue, &msg, TX_NO_WAIT);
                    if (queue_status != TX_SUCCESS)
                    {
                        printf(ANSI_BLUE "[Network Thread] " ANSI_RED "Error: Failed to re-queue message to front (0x%02X).\r\n" ANSI_RESET, queue_status);
                    }
                    break;
                }

                float temp_abs = (msg.temperature < 0.0f) ? -msg.temperature : msg.temperature;
                int temp_dec = (int)temp_abs;
                int temp_frac = (int)((temp_abs - (float)temp_dec) * 100.0f);
                
                int hum_dec = (int)msg.humidity;
                int hum_frac = (int)((msg.humidity - (float)hum_dec) * 100.0f);

                /* Create JSON payload */
                snprintf(payload, sizeof(payload),
                         "{\"temp\":%s%d.%02d,\"hum\":%d.%02d,\"ts\":%lu}",
                         (msg.temperature < 0.0f) ? "-" : "", temp_dec, temp_frac,
                         hum_dec, hum_frac, (unsigned long)msg.timestamp);

                /* Publish telemetry QoS 0 */
                status = nxd_mqtt_client_publish(&mqtt_client, mqtt_topic, strlen(mqtt_topic),
                                                 payload, strlen(payload), 0, 0, 500);
                if (status == NX_SUCCESS)
                {
                    printf(ANSI_BLUE "[Network Thread] " ANSI_GREEN "MQTT Published: %s\r\n" ANSI_RESET, payload);
                }
                else
                {
                    printf(ANSI_BLUE "[Network Thread] " ANSI_RED "Error: Publish failed (0x%02X)\r\n" ANSI_RESET, status);
                    nxd_mqtt_client_disconnect(&mqtt_client);
                    mqtt_connected = NX_FALSE;
                }
            }
        }

        if (mqtt_connected)
        {
            nxd_mqtt_client_disconnect(&mqtt_client);
        }

        mqtt_connected = NX_FALSE;
    }
}

static void status_thread_entry(ULONG thread_input)
{
    (void)thread_input;
    UINT state = 0;

    printf(ANSI_YELLOW "[Status Thread] " ANSI_WHITE "Status Monitor Thread Started.\r\n" ANSI_RESET);

    while (1)
    {
        /* 1. Heartbeat Toggle (Green LED1) */
        if (state)
        {
            LED1_ON();
        }
        else
        {
            LED1_OFF();
        }
        state = !state;

        /* 2. Ethernet Connection Indicator (Blue LED2) */
        if (ip_resolved)
        {
            LED2_ON();
        }
        else
        {
            LED2_OFF();
        }

        /* 3. MQTT Connection / Network Fault Indicator (Red LED3) */
        if (!ip_resolved || !mqtt_connected)
        {
            /* Blink Red LED if we have no connection */
            static UINT red_state = 0;
            if (red_state)
            {
                LED3_ON();
            }
            else
            {
                LED3_OFF();
            }
            red_state = !red_state;
        }
        else
        {
            LED3_OFF();
        }

        /* Sleep 500ms */
        tx_thread_sleep(50);
    }
}
