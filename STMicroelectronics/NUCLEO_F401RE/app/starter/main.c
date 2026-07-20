/*
 * Copyright (c) 2026 Eclipse ThreadX contributors
 *
 * This program and the accompanying materials are made available
 * under the terms of the MIT license which is available at
 * https://opensource.org/license/mit.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "tx_api.h"

#include "board_init.h"
#include "cloud_config.h"
#include "nucleo_bsp.h"

#define THREAD_STACK_SIZE 1024

typedef struct {
  CHAR *name;
  UINT state;
  ULONG run_count;
  UINT priority;
  ULONG stack_used;
  ULONG stack_size;
} ThreadStats;

#define MAX_THREADS 9

typedef struct {
  ULONG timestamp;
  ULONG reading_id;
  ULONG data_value;
} SensorData;

typedef struct {
  ULONG uptime;
  ULONG byte_pool_total;
  ULONG byte_pool_free;
  ULONG byte_pool_used;
  ThreadStats threads[MAX_THREADS];
  UINT thread_count;
} SystemStats;

static SystemStats system_stats = {0};

/* Thread Registry */
static TX_THREAD *thread_registry[MAX_THREADS];

/* Global Byte Pool for dynamic stack allocation */
static TX_BYTE_POOL byte_pool;

/* Thread Control Blocks */
static TX_THREAD monitor_thread;
static TX_THREAD reporter_thread;
static TX_THREAD blink_thread;
static TX_THREAD event_thread;
static TX_THREAD mutex_thread_1;
static TX_THREAD mutex_thread_2;
static TX_THREAD queue_sender_thread;
static TX_THREAD queue_receiver_thread;
static TX_THREAD semaphore_thread;

/* RTOS Primitives */
static TX_MUTEX spi_mutex;
static TX_QUEUE msg_queue;
static TX_EVENT_FLAGS_GROUP event_flags;
static TX_BLOCK_POOL sensor_block_pool;
static TX_SEMAPHORE sensor_semaphore;
static TX_TIMER app_timer;

/* Activity and Primitive Counters */
static ULONG monitor_counter = 0;
static ULONG reporter_counter = 0;
static ULONG blink_counter = 0;

static ULONG mutex_acquires_1 = 0;
static ULONG mutex_acquires_2 = 0;
static ULONG queue_msgs_received = 0;
static ULONG event_flags_processed = 0;
static ULONG semaphore_wakes = 0;
static ULONG timer_counter = 0;

static const char *get_state_string(UINT state) {
  switch (state) {
  case TX_READY:
    return "READY";
  case TX_COMPLETED:
    return "COMPLETED";
  case TX_TERMINATED:
    return "TERMINATED";
  case TX_SUSPENDED:
    return "SUSPENDED";
  case TX_SLEEP:
    return "SLEEP";
  case TX_QUEUE_SUSP:
    return "QUEUE_SUSP";
  case TX_SEMAPHORE_SUSP:
    return "SEMA_SUSP";
  case TX_EVENT_FLAG:
    return "EVENT_FLAG";
  case TX_MUTEX_SUSP:
    return "MUTEX_SUSP";
  default:
    return "BLOCKED";
  }
}

/* Monitor Thread: Producer of statistics */
static void monitor_thread_entry(ULONG parameter) {
  (void)parameter;

  while (1) {
    monitor_counter++;

    /* 1. Track system uptime */
    system_stats.uptime = tx_time_get() / TX_TIMER_TICKS_PER_SECOND;

    /* 2. Track dynamic memory allocation stats */
    CHAR *name;
    ULONG available_bytes;
    ULONG fragments;
    TX_THREAD *suspended_thread;
    ULONG suspended_count;
    TX_BYTE_POOL *next_pool;

    if (tx_byte_pool_info_get(&byte_pool, &name, &available_bytes, &fragments,
                              &suspended_thread, &suspended_count,
                              &next_pool) == TX_SUCCESS) {
      system_stats.byte_pool_total = byte_pool.tx_byte_pool_size;
      system_stats.byte_pool_free = available_bytes;
      system_stats.byte_pool_used =
          byte_pool.tx_byte_pool_size - available_bytes;
    }

    /* 3. Query all registered threads' metrics */
    for (int i = 0; i < MAX_THREADS; i++) {
      TX_THREAD *thread = thread_registry[i];
      if (thread != NULL) {
        CHAR *t_name;
        UINT t_state;
        ULONG t_run_count;
        UINT t_priority;
        UINT preemption_threshold;
        ULONG time_slice;
        TX_THREAD *next_thread;
        TX_THREAD *next_suspended;

        UINT status = tx_thread_info_get(
            thread, &t_name, &t_state, &t_run_count, &t_priority,
            &preemption_threshold, &time_slice, &next_thread, &next_suspended);
        if (status == TX_SUCCESS) {
          system_stats.threads[i].name = t_name;
          system_stats.threads[i].state = t_state;
          system_stats.threads[i].run_count = t_run_count;
          system_stats.threads[i].priority = t_priority;

          /* Scan stack from bottom (tx_thread_stack_start) upward to find first
           * non-0xEF byte */
          ULONG *stack_lowest = (ULONG *)thread->tx_thread_stack_start;
          ULONG *stack_highest = (ULONG *)thread->tx_thread_stack_end;
          ULONG fill_value = 0xEFEFEFEFUL;
          while (stack_lowest <= stack_highest) {
            if (*stack_lowest != fill_value) {
              break;
            }
            stack_lowest++;
          }
          ULONG unused =
              (ULONG)stack_lowest - (ULONG)thread->tx_thread_stack_start;
          system_stats.threads[i].stack_used =
              thread->tx_thread_stack_size - unused;
          system_stats.threads[i].stack_size = thread->tx_thread_stack_size;
        }
      }
    }
    system_stats.thread_count = MAX_THREADS;

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10); /* Run at 10 Hz (100ms) */
  }
}

/* Reporter Thread: Consumer/Printer of statistics */
static void reporter_thread_entry(ULONG parameter) {
  (void)parameter;

  printf("\r\n==========================================\r\n");
  printf("NUCLEO-F401RE Device Monitor Demo\r\n");
  printf("Third-party licensing info in NOTICE.md\r\n");
  printf("==========================================\r\n");

  while (1) {
    reporter_counter++;

    printf("\r\nSystem Status:\r\n");
    printf("------------------------------------------\r\n");
    printf("Uptime:           %lu s\r\n", system_stats.uptime);
    printf("Byte Pool Size:   %lu bytes\r\n", system_stats.byte_pool_total);
    printf("Allocated Memory: %lu bytes\r\n", system_stats.byte_pool_used);
    printf("Free Memory:      %lu bytes\r\n", system_stats.byte_pool_free);
    printf("------------------------------------------\r\n");

    printf("\r\n%-16s %-8s %-10s %-12s %-22s\r\n", "Thread Name", "Priority",
           "State", "Run Count", "Stack Peak (Max / Size)");
    printf("-------------------------------------------------------------------"
           "---------\r\n");
    for (UINT i = 0; i < system_stats.thread_count; i++) {
      if (system_stats.threads[i].name != NULL) {
        ULONG pct = 0;
        if (system_stats.threads[i].stack_size > 0) {
          pct = (system_stats.threads[i].stack_used * 100) /
                system_stats.threads[i].stack_size;
        }
        printf("%-16s %-8u %-10s %-12lu %4lu / %4lu bytes (%lu%%)\r\n",
               system_stats.threads[i].name, system_stats.threads[i].priority,
               get_state_string(system_stats.threads[i].state),
               system_stats.threads[i].run_count,
               system_stats.threads[i].stack_used,
               system_stats.threads[i].stack_size, pct);
      }
    }
    printf("-------------------------------------------------------------------"
           "---------\r\n");

    printf("Runs: Monitor: %lu | Reporter: %lu | Blink: %lu | Timer Wakes: %lu\r\n",
           monitor_counter, reporter_counter, blink_counter, timer_counter);
    printf("RTOS Showcase: Mutex Locks: %lu/%lu | Queue Msgs: %lu | Event Wakes: %lu | Sema Wakes: %lu\r\n",
           mutex_acquires_1, mutex_acquires_2, queue_msgs_received, event_flags_processed, semaphore_wakes);

    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 2); /* Report every 2 seconds */
  }
}

static void blink_thread_entry(ULONG parameter) {
  (void)parameter;

  while (1) {
    nucleo_led_toggle();
    blink_counter++;
    /* Toggle Event Flag bit 0 when LED toggles */
    tx_event_flags_set(&event_flags, 0x01, TX_OR);
    /* Signal Counting Semaphore */
    tx_semaphore_put(&sensor_semaphore);
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2); /* Sleep 500ms */
  }
}

static void event_thread_entry(ULONG parameter) {
  (void)parameter;
  ULONG actual_flags;

  while (1) {
    /* Wait for event flag bit 0 to be set by blink_thread */
    UINT status = tx_event_flags_get(&event_flags, 0x01, TX_AND_CLEAR,
                                     &actual_flags, TX_WAIT_FOREVER);
    if (status == TX_SUCCESS) {
      event_flags_processed++;
    }
  }
}

static void semaphore_thread_entry(ULONG parameter) {
  (void)parameter;

  while (1) {
    /* Block on counting semaphore */
    if (tx_semaphore_get(&sensor_semaphore, TX_WAIT_FOREVER) == TX_SUCCESS) {
      semaphore_wakes++;
    }
  }
}

static void app_timer_callback(ULONG input) {
  (void)input;
  timer_counter++;
}

static void mutex_thread_1_entry(ULONG parameter) {
  (void)parameter;

  while (1) {
    /* Compete for SPI Mutex */
    if (tx_mutex_get(&spi_mutex, TX_WAIT_FOREVER) == TX_SUCCESS) {
      mutex_acquires_1++;
      tx_thread_sleep(5); /* Simulate SPI usage (50ms) */
      tx_mutex_put(&spi_mutex);
    }
    tx_thread_sleep(5); /* Give Mutex Thread 2 a chance (50ms) */
  }
}

static void mutex_thread_2_entry(ULONG parameter) {
  (void)parameter;

  while (1) {
    /* Compete for SPI Mutex */
    if (tx_mutex_get(&spi_mutex, TX_WAIT_FOREVER) == TX_SUCCESS) {
      mutex_acquires_2++;
      tx_thread_sleep(5); /* Simulate SPI usage (50ms) */
      tx_mutex_put(&spi_mutex);
    }
    tx_thread_sleep(5); /* Give Mutex Thread 1 a chance (50ms) */
  }
}

static void queue_sender_entry(ULONG parameter) {
  (void)parameter;
  ULONG msg_val = 0;

  while (1) {
    msg_val++;
    SensorData *data_ptr;
    /* Allocate block from fixed-size block pool */
    if (tx_block_allocate(&sensor_block_pool, (VOID **)&data_ptr, TX_NO_WAIT) == TX_SUCCESS) {
      data_ptr->timestamp = tx_time_get();
      data_ptr->reading_id = msg_val;
      data_ptr->data_value = msg_val * 42;
      
      /* Send pointer containing SensorData */
      if (tx_queue_send(&msg_queue, &data_ptr, TX_NO_WAIT) != TX_SUCCESS) {
        /* Release block if queue was full to prevent leak */
        tx_block_release(data_ptr);
      }
    }
    tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 5); /* Send every 200ms */
  }
}

static void queue_receiver_entry(ULONG parameter) {
  (void)parameter;
  SensorData *rx_data_ptr;

  while (1) {
    /* Block on queue waiting for messages */
    UINT status = tx_queue_receive(&msg_queue, &rx_data_ptr, TX_WAIT_FOREVER);
    if (status == TX_SUCCESS) {
      queue_msgs_received++;
      /* Release block back to block pool */
      tx_block_release(rx_data_ptr);
    }
  }
}

void tx_application_define(void *first_unused_memory) {
  UINT status;
  CHAR *stack_ptr;
  ULONG pool_size;

  /* Calculate available RAM for the byte pool, leaving 4KB margin for system
   * stack at the top (0x20018000) */
  pool_size = (0x20018000 - 4096) - (ULONG)first_unused_memory;

  /* Initialize the byte pool */
  status = tx_byte_pool_create(&byte_pool, "system byte pool",
                               first_unused_memory, pool_size);
  if (status != TX_SUCCESS) {
    printf("Byte pool create failed: 0x%08x\r\n", status);
    return;
  }

  /* Create Mutex */
  status = tx_mutex_create(&spi_mutex, "SPI Mutex", TX_NO_INHERIT);
  if (status != TX_SUCCESS) {
    printf("Mutex create failed: 0x%08x\r\n", status);
    return;
  }

  /* Create Event Flags Group */
  status = tx_event_flags_create(&event_flags, "Event Flags");
  if (status != TX_SUCCESS) {
    printf("Event flags create failed: 0x%08x\r\n", status);
    return;
  }

  /* Create Counting Semaphore */
  status = tx_semaphore_create(&sensor_semaphore, "Sensor Semaphore", 0);
  if (status != TX_SUCCESS) {
    printf("Semaphore create failed: 0x%08x\r\n", status);
    return;
  }

  /* Create Application Timer */
  status = tx_timer_create(&app_timer, "App Timer", app_timer_callback, 0,
                           TX_TIMER_TICKS_PER_SECOND, TX_TIMER_TICKS_PER_SECOND,
                           TX_AUTO_START);
  if (status != TX_SUCCESS) {
    printf("Timer create failed: 0x%08x\r\n", status);
    return;
  }

  /* Allocate Block Pool memory and create Block Pool */
  VOID *block_pool_ptr;
  ULONG block_pool_size = 10 * (sizeof(SensorData) + sizeof(VOID *)) + 32;
  status = tx_byte_allocate(&byte_pool, &block_pool_ptr, block_pool_size, TX_NO_WAIT);
  if (status != TX_SUCCESS) {
    printf("Block pool memory allocate failed: 0x%08x\r\n", status);
    return;
  }
  status = tx_block_pool_create(&sensor_block_pool, "Sensor Block Pool", sizeof(SensorData),
                                block_pool_ptr, block_pool_size);
  if (status != TX_SUCCESS) {
    printf("Block pool create failed: 0x%08x\r\n", status);
    return;
  }

  /* Allocate Queue memory and create Queue (Stores 10 SensorData pointers) */
  VOID *queue_buffer_ptr;
  status = tx_byte_allocate(&byte_pool, &queue_buffer_ptr, 10 * sizeof(ULONG),
                            TX_NO_WAIT);
  if (status != TX_SUCCESS) {
    printf("Queue memory allocate failed: 0x%08x\r\n", status);
    return;
  }
  status = tx_queue_create(&msg_queue, "Msg Queue", TX_1_ULONG,
                           queue_buffer_ptr, 10 * sizeof(ULONG));
  if (status != TX_SUCCESS) {
    printf("Queue create failed: 0x%08x\r\n", status);
    return;
  }

  /* Allocate stack and create Monitor Thread (Priority 9) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(
        &monitor_thread, "monitor thread", monitor_thread_entry, 0, stack_ptr,
        THREAD_STACK_SIZE, 9, 9, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Monitor thread create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Reporter Thread (Priority 10) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(
        &reporter_thread, "reporter thread", reporter_thread_entry, 0,
        stack_ptr, THREAD_STACK_SIZE, 10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Reporter thread create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Blink Thread (Priority 11) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(&blink_thread, "blink thread", blink_thread_entry,
                              0, stack_ptr, THREAD_STACK_SIZE, 11, 11,
                              TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Blink thread create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Event Thread (Priority 12) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(&event_thread, "event thread", event_thread_entry,
                              0, stack_ptr, THREAD_STACK_SIZE, 12, 12,
                              TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Event thread create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Semaphore Thread (Priority 12) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(&semaphore_thread, "semaphore thread", semaphore_thread_entry,
                              0, stack_ptr, THREAD_STACK_SIZE, 12, 12,
                              TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Semaphore thread create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Mutex Thread 1 (Priority 13) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(
        &mutex_thread_1, "mutex thread 1", mutex_thread_1_entry, 0, stack_ptr,
        THREAD_STACK_SIZE, 13, 13, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Mutex thread 1 create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Mutex Thread 2 (Priority 13) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(
        &mutex_thread_2, "mutex thread 2", mutex_thread_2_entry, 0, stack_ptr,
        THREAD_STACK_SIZE, 13, 13, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Mutex thread 2 create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Queue Sender Thread (Priority 14) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(
        &queue_sender_thread, "queue sender", queue_sender_entry, 0, stack_ptr,
        THREAD_STACK_SIZE, 14, 14, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Queue sender create failed: 0x%08x\r\n", status);
    }
  }

  /* Allocate stack and create Queue Receiver Thread (Priority 15) */
  status = tx_byte_allocate(&byte_pool, (VOID **)&stack_ptr, THREAD_STACK_SIZE,
                            TX_NO_WAIT);
  if (status == TX_SUCCESS) {
    status = tx_thread_create(
        &queue_receiver_thread, "queue receiver", queue_receiver_entry, 0,
        stack_ptr, THREAD_STACK_SIZE, 15, 15, TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) {
      printf("Queue receiver create failed: 0x%08x\r\n", status);
    }
  }

  /* Save pointers to thread control blocks in the registry */
  thread_registry[0] = &monitor_thread;
  thread_registry[1] = &reporter_thread;
  thread_registry[2] = &blink_thread;
  thread_registry[3] = &event_thread;
  thread_registry[4] = &mutex_thread_1;
  thread_registry[5] = &mutex_thread_2;
  thread_registry[6] = &queue_sender_thread;
  thread_registry[7] = &queue_receiver_thread;
  thread_registry[8] = &semaphore_thread;
}

int main(void) {
  board_init();

  /* Start the ThreadX kernel */
  tx_kernel_enter();

  while (1) {
  }
}
